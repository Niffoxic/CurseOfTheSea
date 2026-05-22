// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PASS_H
#define CURSEOFTHESEA_PASS_H

#include <cstdint>
#include <cstddef>

#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/render_snapshot.h"

struct ID3D12Resource2;

namespace cots::graphics
{
    namespace hardware { class device;       }
    namespace shaders  { class shader_cache; }

    struct setup_context
    {
        hardware::device&       device;
        shaders::shader_cache&  shaders;
    };

    //~ everything a pass needs for one frame of recording
    struct pass_context
    {
        hardware::command_context& ctx;
        const scene_snapshot&      snap;

        ID3D12Resource2*           backbuffer;   //~ TODO: generalizes to named resources later
        std::size_t                rtv_handle;

        std::uint32_t              width;
        std::uint32_t              height;
        std::uint32_t              frame_index;
    };

    class pass
    {
    public:
        virtual ~pass() = default;

        pass()                       = default;
        pass(const pass&)            = delete;
        pass(pass&&)                 = delete;
        pass& operator=(const pass&) = delete;
        pass& operator=(pass&&)      = delete;

        //~ one-time GPU resource creation runs on the RT
        virtual bool setup(const setup_context& sc) { (void)sc; return true; }

        //~ per-frame recording
                      virtual       void execute(const pass_context& pc) = 0;
        [[nodiscard]] virtual const char* name  () const noexcept        = 0;
    };
}

#endif //CURSEOFTHESEA_PASS_H
