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
#ifndef CURSEOFTHESEA_BUFFER_MANAGER_H
#define CURSEOFTHESEA_BUFFER_MANAGER_H

#include <atomic>
#include <cstdint>
#include <vector>
#include <wrl/client.h>
#include <span>

#include "resource.h"
#include "trishul/core/interface/hardware.h"

struct ID3D12Resource;

namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class device;
    class deferred_releaser;
    class upload_arena;

    struct buffer_create_info
    {
        std::uint64_t   size_bytes { 0 };
        buffer_kind     kind       { buffer_kind::generic };
        const void*     initial_data { nullptr };   //~ optional staged upload if set
        std::uint64_t   stride     { 0 };           //~ vertex size for vertex buffers
        const char*     debug_name { "buffer" };
    };

    //~ what the maker needs wired before initialization
    struct buffer_manager_config
    {
        const device*      dev      { nullptr };
        deferred_releaser* releaser { nullptr };
        upload_arena*      arena    { nullptr };
    };

    //~ makes and hands out gpu buffers vertex index constant uav whatever the
    //~ pixels for the static ones get pushed up through the shared arena it is
    //~ a hardware child so the handler builds and tears it down in order
    class buffer_manager final: public interfaces
    {
    public:
         buffer_manager() = default;
        ~buffer_manager() override;

        buffer_manager           (const buffer_manager&) = delete;
        buffer_manager& operator=(const buffer_manager&) = delete;

        //~ lifecycle
        [[nodiscard]] bool initialize  ()          override;
                      void deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "buffer_manager"; }

        //~ flag a rebuild the handler will bounce us
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ late binding overrides the config usually carries these already so
        //~ you rarely reach for them by hand
        void set_releaser    (deferred_releaser* r) noexcept { releaser_ = r; }
        void set_upload_arena(upload_arena* a)      noexcept { arena_ = a; }

        //~ uploads run synchronously on the
        [[nodiscard]] buffer_handle create(const buffer_create_info& info);

        //~ one flush for many buffers
        [[nodiscard]]
        std::vector<buffer_handle> create_batch(std::span<const buffer_create_info> infos);

        void destroy(buffer_handle h);

        //~ accessors for binding
        [[nodiscard]] ID3D12Resource*           resource(buffer_handle h) const;
        [[nodiscard]] std::uint64_t             gpu_address(buffer_handle h) const;
        [[nodiscard]] std::uint64_t             size(buffer_handle h) const;
        [[nodiscard]] std::uint64_t             stride(buffer_handle h) const;

        //~ persistently-mapped CPU pointer for per-frame writes
        [[nodiscard]] void*                     mapped_ptr(buffer_handle h) const;

    private:
        struct slot
        {
            D3D12MA::Allocation* allocation { nullptr };
            ID3D12Resource*      resource   { nullptr };
            std::uint64_t        size       { 0 };
            std::uint64_t        stride     { 0 };
            void*                mapped     { nullptr };  //~ constant buffers only
            buffer_kind          kind       { buffer_kind::generic };
        };

        bool upload_static(const slot& s, const void* data, std::uint64_t size) const;

        //~ build a buffer on a local slot no map entry yet returns false and
        //~ leaves out cleaned up on failure caller owns out either way
        [[nodiscard]] bool allocate_only(const buffer_create_info& info, slot& out);

        //~ payload for the batched upload dst points at a local slot that stays
        //~ put until we insert so the pointer is safe for the whole call
        struct upload_record
        {
            slot*         dst;
            const void*   data;
            std::uint64_t size;
        };
        bool upload_batch(std::span<const upload_record> records) const;

        //~ free everything a slot owns right now unmap then drop both refs only
        //~ safe when the gpu is idle teardown or cleaning up a half built buffer
        static void release_slot_now(slot& s) noexcept;

    private:
        const device*       device_   { nullptr };
        deferred_releaser*  releaser_ { nullptr };
        upload_arena*       arena_    { nullptr };

        //~ slot_map handles index generation free list and stale checks we just
        //~ keep the gpu release logic since that has to be deferral aware
        slot_map<slot, buffer_tag> buffers_;
        std::atomic<bool>          need_rebuild_{ true };
    };
} // namespace trishul::render::hardware

#endif
