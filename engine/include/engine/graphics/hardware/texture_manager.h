// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TEXTURE_MANAGER_H
#define CURSEOFTHESEA_TEXTURE_MANAGER_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>

struct ID3D12Resource2;

namespace D3D12MA { class Allocation; }

namespace cots::graphics::hardware
{
    class device;
    class descriptor_heap;

    struct texture_handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]] bool valid() const noexcept
        {
            return generation != 0u;
        }

        [[nodiscard]] static texture_handle invalid() noexcept
        {
            return { 0u, 0u };
        }

        bool operator==(const texture_handle& o) const noexcept
        {
            return index == o.index && generation == o.generation;
        }
    };

    enum class texture_format : std::uint8_t
    {
        rgba8_unorm,
        rgba8_unorm_srgb,
    };

    struct texture_create_info
    {
        std::uint32_t   width        { 0 };
        std::uint32_t   height       { 0 };
        texture_format  format       { texture_format::rgba8_unorm };
        const void*     pixels       { nullptr }; //~ packed rgba source
        std::uint32_t   row_pitch    { 0 };       //~ zero means tight rows
        const char*     debug_name   { "texture" };
    };

    //~ creates sampleable textures
    //~ uploads and registers srvs
    class texture_manager final
    {
    public:
         texture_manager() = default;
        ~texture_manager();

        texture_manager           (const texture_manager&) = delete;
        texture_manager& operator=(const texture_manager&) = delete;

        [[nodiscard]] bool initialize  (device& dev, descriptor_heap& bindless);
                      void deinitialize() noexcept;

        [[nodiscard]] texture_handle create(const texture_create_info& info);
                      void           destroy(texture_handle h);

        [[nodiscard]] ID3D12Resource2* resource     (texture_handle h) const;
        [[nodiscard]] std::uint32_t    bindless_slot(texture_handle h) const;
        [[nodiscard]] std::uint32_t    width        (texture_handle h) const;
        [[nodiscard]] std::uint32_t    height       (texture_handle h) const;

    private:
        struct slot
        {
            D3D12MA::Allocation* allocation    { nullptr };
            ID3D12Resource2*     resource      { nullptr };
            std::uint32_t        width         { 0 };
            std::uint32_t        height        { 0 };
            std::uint32_t        bindless_slot { 0 };
            std::uint32_t        generation    { 0 };
            texture_format       format        { texture_format::rgba8_unorm };
        };

        [[nodiscard]] std::uint32_t acquire_slot();
        bool upload_pixels(ID3D12Resource2* dst,
                           std::uint32_t width, std::uint32_t height,
                           const void* pixels, std::uint32_t row_pitch) const;
        void create_srv  (slot& s);

    private:
        device*          device_   { nullptr };
        descriptor_heap* bindless_ { nullptr };

        std::vector<slot> slots_;
        std::uint32_t     next_generation_ { 1 };
    };
} // namespace cots::graphics::hardware

#endif //CURSEOFTHESEA_TEXTURE_MANAGER_H
