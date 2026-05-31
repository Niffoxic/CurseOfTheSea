//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#ifndef CURSEOFTHESEA_UPLOAD_ARENA_H
#define CURSEOFTHESEA_UPLOAD_ARENA_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>
#include <wrl/client.h>
#include <windows.h>
#include <d3d12.h>

#include "trishul/core/interface/hardware.h"

namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class device;

    struct upload_arena_config
    {
        const device* dev{ nullptr };
    };

    //~ pool and batch recorder for gpu uploads for replacing the per buffer
    // creation staging blob submit signal wait release soo on... cycle
    // with a single command list a fence
    class upload_arena final: public interfaces
    {
    public:
        using fence_value = std::uint64_t;

        upload_arena() = default;
        ~upload_arena() override;

        upload_arena           (const upload_arena&) = delete;
        upload_arena& operator=(const upload_arena&) = delete;

        //~ lifecycle
        [[nodiscard]] bool initialize  ()          override;
        void               deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "upload_arena"; }

        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ reset the cmd allocator and list ready to record a new batch
        // assuming the prior batch has completed in sync mode
        bool begin_batch();

        //~ enqueue a linear buffer copy size in bytes the arena copies
        // data into a pooled staging buffer and records copy buffer region
        // queues a post copy barrier common to final_state
        bool add_buffer_copy(ID3D12Resource*       dst,
                             const void*           data,
                             std::uint64_t         size,
                             D3D12_RESOURCE_STATES final_state);

        //~ enqueue a single mip 2d texture copy
        bool add_texture2d_copy(ID3D12Resource*  dst,
                                std::uint32_t    width,
                                std::uint32_t    height,
                                const void*      pixels,
                                std::uint32_t    source_row_pitch);

        //~ enqueue a full mip chain copy for  every subresource expects
        // prepare upload style subresource data
        // a single pre and post barrier covers all sub resources
        bool add_texture_subresources(
            ID3D12Resource*                         dst,
            std::span<const D3D12_SUBRESOURCE_DATA> subresources);

        //~ close the cmd list submit on the device graphics queue
        //~ returns fence value indicating post completion
        [[nodiscard]] fence_value submit();

        //~ thread safe block until the gpu signals
        bool wait(fence_value v) const;

        //~ thread safe non blocking poll
        [[nodiscard]] bool is_complete(fence_value v) const;

        //~ sync helper begin batch is assumed already called returns true
        // on success the arena has already waited and recycled
        bool submit_and_wait();

        //~ throw away the open batch returns queued staging to the pool
        // without submitting
        void cancel_batch();

        //~ only move in flight staging whose stamp is
        // reached on the gpu side back to the free pool called from
        // begin batch and once a frame from the renderer to reclaim
        // afterward without any wait
        std::size_t recycle_completed();

        //~ debugging stats TODO: gotta supply this to the end user
        [[nodiscard]] std::size_t   free_count     () const noexcept;
        [[nodiscard]] std::size_t   in_flight_count() const noexcept;
        [[nodiscard]] std::uint64_t reused_count   () const noexcept { return reused_;    }
        [[nodiscard]] std::uint64_t allocated_count() const noexcept { return allocated_; }

    private:
        struct staging
        {
            D3D12MA::Allocation*                    allocation { nullptr };
            Microsoft::WRL::ComPtr<ID3D12Resource>  resource;
            std::uint8_t*                           mapped     { nullptr };
            std::uint64_t                           size       { 0 };

            staging()                          = default;
            staging(const staging&)            = delete;
            staging& operator=(const staging&) = delete;
            staging(staging&& o) noexcept;
            staging& operator=(staging&& o) noexcept;
            ~staging();

            //~ helper releases the allocation and the mapping
            void destroy();
        };

        struct in_flight_entry
        {
            staging      buf;
            fence_value  stamp { 0 };
        };

        struct buffer_copy_record
        {
            ID3D12Resource* dst;
            ID3D12Resource* staging_resource;
            std::uint64_t   staging_offset;
            std::uint64_t   size;
        };

        struct texture_copy_record
        {
            D3D12_TEXTURE_COPY_LOCATION src;
            D3D12_TEXTURE_COPY_LOCATION dst;
        };

        struct tex_pre_barrier_record
        {
            ID3D12Resource* texture;
        };

        struct tex_post_barrier_record
        {
            ID3D12Resource* texture;
        };

        struct buf_post_barrier_record
        {
            ID3D12Resource*       buffer;
            D3D12_RESOURCE_STATES after;
        };

        //~ acquire a staging buffer of at least size bytes either from the
        // free pool or by allocating a new one returns by move into out
        bool acquire_staging(std::uint64_t size, staging& out);

        //~ allocate a fresh persistently mapped upload heap buffer
        bool allocate_staging(std::uint64_t size, staging& out);

        const device*                                      device_ { nullptr };
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     cmd_alloc_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmd_list_;
        Microsoft::WRL::ComPtr<ID3D12Fence1>               fence_;

        std::atomic<fence_value> next_signal_   { 1u };

        //~ pool state render thread only
        std::vector<staging>         free_pool_;
        std::vector<staging>         in_use_;
        std::vector<in_flight_entry> in_flight_;

        //~ batch op queues render thread only
        std::vector<buffer_copy_record>      buffer_copies_;
        std::vector<texture_copy_record>     texture_copies_;
        std::vector<tex_pre_barrier_record>  pre_tex_barriers_;
        std::vector<tex_post_barrier_record> post_tex_barriers_;
        std::vector<buf_post_barrier_record> post_buf_barriers_;

        //~ stats
        std::uint64_t reused_    { 0 };
        std::uint64_t allocated_ { 0 };

        bool              batch_open_  { false };
        std::atomic<bool> need_rebuild_{ true };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_UPLOAD_ARENA_H
