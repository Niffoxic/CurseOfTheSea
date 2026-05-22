// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/feature_locator.h"
#include "engine/graphics/render.h"

namespace
{
    auto render()
    {
        return cots::feature::locator::resolve<cots::graphics::render>();
    }

    void set_camera(const float view[16], const float proj[16],
                    const float pos[3], const float fwd[3], const float up[3])
    {
        auto& cam = render()->building_snapshot().camera;
        std::memcpy(&cam.view,       view, sizeof(float) * 16);
        std::memcpy(&cam.projection, proj, sizeof(float) * 16);
        cam.position = { pos[0], pos[1], pos[2] };
        cam.forward  = { fwd[0], fwd[1], fwd[2] };
        cam.up       = { up[0],  up[1],  up[2]  };
    }

    void submit_instance(const int mesh_id, const float world[16], const int material_id)
    {
        auto& snap = render()->building_snapshot();
        auto& inst = snap.instances.emplace_back();
        std::memcpy(&inst.transform, world, sizeof(float) * 16);
        inst.mesh_index     = static_cast<std::uint32_t>(mesh_id);
        inst.material_index = static_cast<std::uint32_t>(material_id);
    }

    void install(cots::module::services& s)
    {
        s.render.set_camera      = &set_camera;
        s.render.submit_instance = &submit_instance;
    }
} // namespace anonymouse

COTS_INSTALL_SERVICES(install)
