// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PASS_H
#define CURSEOFTHESEA_PASS_H

#include <cstdint>

#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/render_snapshot.h"
#include "engine/graphics/graph/resource_registry.h"
#include "engine/graphics/graph/declare_context.h"

namespace cots::graphics
{
    namespace hardware
    {
        class device;
        class buffer_manager;
        class texture_manager;
        class descriptor_heap;
    } // namespace hardware

    namespace shaders{ class shader_cache;  }
    namespace meshes { class mesh_registry; }

    struct setup_context
    {
        hardware::device&          device;
        shaders::shader_cache&     shaders;
        hardware::buffer_manager&  buffers;
        meshes::mesh_registry&     meshes;
        hardware::texture_manager& textures;
        hardware::descriptor_heap& bindless;
    };

    //~ everything a pass needs for one frame of recording
    // graph owned resources, the pass looks up
    // its handles to get the current pe -frame view (basically resource + RTV/DSV)
    struct pass_context
    {
        hardware::command_context&        ctx;
        const scene_snapshot&             snap;
        const graph::resource_registry&   resources;

        std::uint32_t                     width;
        std::uint32_t                     height;
        std::uint32_t                     frame_index;
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
        virtual bool setup(const setup_context& sc)
        {
            (void)sc;
            return true;
        }

        //~ advertise resource reads or writes and their usage kinds to graph
        virtual void declare(graph::declare_context& dc) { (void)dc; }

        //~ per-frame recording
                      virtual       void execute(const pass_context& pc) = 0;
        [[nodiscard]] virtual const char* name  () const noexcept        = 0;
    };
}

#endif //CURSEOFTHESEA_PASS_H
