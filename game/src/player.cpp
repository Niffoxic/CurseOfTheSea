// Created by Niffoxic (Harsh Dubey)
#include "player.h"
#include "game_state.h"

#include <DirectXMath.h>
#include <cots/engine_services.h>

namespace game
{
    void player_init(game_state& gs)
    {
        gs.player = {};
        gs.services->log.info("player: initialized");
    }

    void player_update(game_state& gs, const float dt)
    {
        const auto& in = gs.services->input;
        auto& p = gs.player;

        const float v = p.speed * dt;
        if (in.key_down('W')) p.pos[2] += v;
        if (in.key_down('S')) p.pos[2] -= v;
        if (in.key_down('A')) p.pos[0] -= v;
        if (in.key_down('D')) p.pos[0] += v;

        // sprint with shift
        if (in.shift_down())
        {
            if (in.key_down('W')) p.pos[2] += v;
            if (in.key_down('S')) p.pos[2] -= v;
            if (in.key_down('A')) p.pos[0] -= v;
            if (in.key_down('D')) p.pos[0] += v;
        }

        using namespace DirectX;

        int w = 1280, h = 720;
        gs.services->window.get_size(&w, &h);
        const float aspect = h ? static_cast<float>(w) / static_cast<float>(h) : 1.7778f;

        //~ camera sits back on -Z
        const XMVECTOR eye = XMVectorSet(p.pos[0], p.pos[1] + 1.0f, p.pos[2] - 6.0f, 1.0f);
        const XMVECTOR at  = XMVectorSet(p.pos[0], p.pos[1],        p.pos[2],        1.0f);
        const XMVECTOR up  = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        const XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
        //~ updated for far and near z since its reversed now
        const XMMATRIX proj = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(60.0f),
            aspect, 1000.0f,
            0.1f
        );

        XMFLOAT4X4 v4, p4;
        XMStoreFloat4x4(&v4, view);
        XMStoreFloat4x4(&p4, proj);

        const auto& r = gs.services->render;
        if (r.set_camera)
        {
            r.set_camera(&v4.m[0][0], &p4.m[0][0], p.pos, p.forward, p.up);
        }

        if (r.submit_instance)
        {
            //~ rotating cube proves self occlusion
            const float angle = gs.elapsed * 0.8f;
            XMFLOAT4X4 cube_world;
            XMStoreFloat4x4(&cube_world,
                XMMatrixRotationRollPitchYaw(angle * 0.7f, angle, 0.0f) *
                XMMatrixTranslation(0.0f, 0.0f, 0.0f));
            r.submit_instance(cots::module::mesh_id_cube, &cube_world.m[0][0], 0);

            XMFLOAT4X4 far_world;
            XMStoreFloat4x4(&far_world,
                XMMatrixScaling(20.0f, 20.0f, 20.0f) *
                XMMatrixTranslation(0.0f, 0.0f, 800.0f));
            r.submit_instance(cots::module::mesh_id_cube, &far_world.m[0][0], 0);

            for (int i = -1; i <= 1; ++i)
            {
                const float s = (i == 0) ? 1.5f : 1.0f;   //~ centre one bigger
                XMFLOAT4X4 world;
                XMStoreFloat4x4(&world,
                    XMMatrixScaling(s, s, 1.0f) *
                    XMMatrixTranslation(static_cast<float>(i) * 2.5f, -1.5f, 2.0f));
                r.submit_instance(cots::module::mesh_id_quad, &world.m[0][0], 0);
            }

            //~ ship from gltf next to the player
            XMFLOAT4X4 ship_world;
            XMStoreFloat4x4(&ship_world,
                XMMatrixRotationY(angle * 0.5f) *
                XMMatrixTranslation(3.0f, 0.0f, 0.0f));
            r.submit_instance(cots::module::mesh_id_ship, &ship_world.m[0][0], 0);
        }

        // push listener so audio tracks the player
        gs.services->audio.set_listener(p.pos, p.forward, p.up);
    }
} // namespace game
