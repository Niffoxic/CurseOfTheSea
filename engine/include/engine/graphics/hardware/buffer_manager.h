// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_BUFFER_MANAGER_H
#define CURSEOFTHESEA_BUFFER_MANAGER_H

#include <cstdint>
#include <memory>
#include <vector>
#include <span>

#include "resource.h"

struct ID3D12Resource;

namespace cots::graphics::hardware
{
    class device;
    class deferred_releaser;
    class upload_arena;

    struct buffer_create_info
    {
        std::uint64_t   size_bytes { 0 };
        buffer_kind     kind       { buffer_kind::generic };
        const void*     initial_data { nullptr };   //~ staged upload if set
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

        [[nodiscard]] bool initialize  (device& dev) const;
                      void deinitialize() const noexcept;


        void set_releaser    (deferred_releaser* r) const noexcept;
        void set_upload_arena(upload_arena* a) const noexcept;

        //~ uploads run synchronously on the
        [[nodiscard]]
        buffer_handle create(const buffer_create_info& info) const;

        //~ one flush for many buffers
        [[nodiscard]]
        std::vector<buffer_handle> create_batch(std::span<const buffer_create_info> infos) const;

        void destroy(buffer_handle h) const;

        //~ accessors for binding
        [[nodiscard]] ID3D12Resource* resource   (buffer_handle h) const;
        [[nodiscard]] std::uint64_t   gpu_address(buffer_handle h) const;
        [[nodiscard]] std::uint64_t   size       (buffer_handle h) const;
        [[nodiscard]] std::uint64_t   stride     (buffer_handle h) const;
        [[nodiscard]] void*           mapped_ptr (buffer_handle h) const;

    private:
        class implementation;
        std::unique_ptr<implementation> impl_;
    };
} // namespace cots::graphics::hardware

#endif
