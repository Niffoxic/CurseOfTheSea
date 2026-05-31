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
#ifndef CURSEOFTHESEA_TEXTURE_MANAGER_H
#define CURSEOFTHESEA_TEXTURE_MANAGER_H

#include <atomic>
#include <cstdint>
#include <vector>
#include <wrl/client.h>
#include <dxgiformat.h>

#include "resource.h"
#include "trishul/core/interface/hardware.h"

struct ID3D12Resource2;

namespace D3D12MA { class Allocation; }

namespace trishul::render::hardware
{
    class device;
    class descriptor_heap;
    class deferred_releaser;
    class upload_arena;

    //~ whatever manager needs for initialization will be handed by the
    //  handler
    struct texture_manager_config
    {
        const device*      dev      { nullptr };
        descriptor_heap*   bindless { nullptr };
        deferred_releaser* releaser { nullptr };
        upload_arena*      arena    { nullptr };
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

    //~ baked dds container payload
    struct dds_create_info
    {
        const void*  dds_data   { nullptr };
        std::size_t  dds_size   { 0 };
        const char*  debug_name { "baked" };
    };

    //~ makes textures so that we can sample in a shader it then creates the gpu resource
    //~ pushes the pixels up through the shared upload arena then parks an srv
    //~ in the bindless heap and hands you back a handle to refer to it later
    class texture_manager final: public interfaces
    {
    public:
         texture_manager() = default;
        ~texture_manager() override;

        texture_manager           (const texture_manager&) = delete;
        texture_manager& operator=(const texture_manager&) = delete;

        //~ lifecycle
        [[nodiscard]] bool initialize  ()          override;
                      void deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "texture_manager"; }

        //~ flag a rebuild the handler will recreate it if parent changed their props
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ late binding overrides for the two optional directions normally the
        //~ config already carries them so dont worry about passing them individually
        void set_releaser    (deferred_releaser* r) noexcept { releaser_ = r; }
        void set_upload_arena(upload_arena* a)      noexcept { arena_ = a; }

        //~ live rgba path
        [[nodiscard]] texture_handle create(const texture_create_info& info);

        //~ baked dds path
        [[nodiscard]] texture_handle create_from_dds(const dds_create_info& info);

                      void             destroy      (texture_handle h);
        [[nodiscard]] ID3D12Resource2* resource     (texture_handle h) const;
        [[nodiscard]] std::uint32_t    bindless_slot(texture_handle h) const;
        [[nodiscard]] std::uint32_t    width        (texture_handle h) const;
        [[nodiscard]] std::uint32_t    height       (texture_handle h) const;
        [[nodiscard]] std::uint32_t    mip_levels   (texture_handle h) const;

    private:
        struct slot
        {
            D3D12MA::Allocation* allocation    { nullptr };
            ID3D12Resource2*     resource      { nullptr };
            std::uint32_t        width         { 0 };
            std::uint32_t        height        { 0 };
            std::uint32_t        mip_levels    { 1 };
            std::uint32_t        bindless_slot { 0 };
            std::uint32_t        generation    { 0 };
            DXGI_FORMAT          dxgi_format   { DXGI_FORMAT_UNKNOWN };
        };

        [[nodiscard]] std::uint32_t acquire_slot();

        //~ single mip upload
        bool upload_pixels(ID3D12Resource2* dst,
                           std::uint32_t width, std::uint32_t height,
                           const void* pixels, std::uint32_t row_pitch) const;

        //~ mip chain upload
        bool upload_dds   (ID3D12Resource2* dst,
                           const void* dds_data, std::size_t dds_size,
                           std::uint32_t mip_levels) const;

        void create_srv   (const slot& s) const;

        //~ drops the gpu objects a slot owns right now no deferral so only call
        //~ this when the gpu is known idle teardown or a failed create cleanup
        static void release_slot_now(slot& s, descriptor_heap* bindless) noexcept;

    private:
        const device*       device_   { nullptr }; //~ reading only from the allocator
        descriptor_heap*    bindless_ { nullptr };
        deferred_releaser*  releaser_ { nullptr };
        upload_arena*       arena_    { nullptr };

        std::vector<slot> slots_;
        std::uint32_t     next_generation_ { 1 };
        std::atomic<bool> need_rebuild_    { true }; //~ flag for handler in case parent changed internals
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_TEXTURE_MANAGER_H
