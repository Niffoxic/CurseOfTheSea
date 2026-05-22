// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_BUFFER_MANAGER_H
#define CURSEOFTHESEA_BUFFER_MANAGER_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>
#include <span>

#include "resource.h"

struct ID3D12Resource;

namespace D3D12MA { class Allocation; }

namespace cots::graphics::hardware
{
    class device;

    struct buffer_create_info
    {
        std::uint64_t   size_bytes { 0 };
        buffer_kind     kind       { buffer_kind::generic };
        const void*     initial_data { nullptr };   //~ optional - staged upload if set
        std::uint64_t   stride     { 0 };           //~ for vertex buffers vertex size
        const char*     debug_name { "buffer" };
    };

    class buffer_manager final
    {
    public:
         buffer_manager() = default;
        ~buffer_manager();

        buffer_manager           (const buffer_manager&) = delete;
        buffer_manager& operator=(const buffer_manager&) = delete;

        [[nodiscard]] bool initialize  (device& dev);
                      void deinitialize() noexcept;

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
            ID3D12Resource*      resource   { nullptr };  //~ owned via allocation
            std::uint64_t        size       { 0 };
            std::uint64_t        stride     { 0 };
            void*                mapped     { nullptr };  //~ constant buffers only
            std::uint32_t        generation { 0 };        //~ 0 = free slot
            buffer_kind          kind       { buffer_kind::generic };
        };

        [[nodiscard]] std::uint32_t acquire_slot();
        bool upload_static(const slot& s, const void* data, std::uint64_t size) const;

        //~ allocate without uploading
        struct allocation_result
        {
            std::uint32_t index { 0 };
            bool          ok    { false };
        };
        [[nodiscard]] allocation_result allocate_only(const buffer_create_info& info);

        //~ payload for the batched upload
        struct upload_record
        {
            slot*         dst;
            const void*   data;
            std::uint64_t size;
        };
        bool upload_batch(std::span<const upload_record> records) const;

    private:
        device*           device_ { nullptr };
        std::vector<slot> slots_;
        std::uint32_t     next_generation_ { 1 };
    };
} // namespace cots::graphics::hardware

#endif
