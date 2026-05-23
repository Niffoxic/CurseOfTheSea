// Created by Niffoxic (Harsh Dubey)

#include "engine/graphics/render.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/resource/depth_target.h"

#include "engine/utils/profiler.h"

#include "engine/core/cots_assert.h"
#include "engine/system/define_features.h"
#include "spdlog/spdlog.h"

#include "engine/events/event_dispatcher.h"
#include "engine/events/windows_event.h"
#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <dxgi1_6.h>

//~ render graph
#include "engine/graphics/passes/pass.h"
#include "engine/graphics/passes/clear_pass.h"
#include "engine/graphics/passes/present_pass.h"
#include "engine/graphics/passes/triangle_pass.h"
#include "engine/graphics/passes/mesh_pass.h"
#include "engine/graphics/meshes/mesh_registry.h"
#include "engine/graphics/graph/render_graph.h"

//~ test
#include "engine/graphics/shaders/storage/binary_storage.h"
#include "engine/graphics/shaders/storage/json_storage.h"
#include "engine/graphics/utils/image_decode.h"
#include "engine/graphics/textures/texture_cache.h"
#include "engine/graphics/textures/texture_intent.h"
#include <cots/cots_config.h>

#include "engine/graphics/meshes/mesh_cache.h"
#include "engine/graphics/meshes/gltf_importer.h"
#include "engine/graphics/meshes/imported_model.h"

#if COTS_EDITOR_ENABLED
#include "editor/editor.h"
#endif

cots::graphics::render::~render() = default;

bool cots::graphics::render::initialize()
{
    subscribe_events();
    running_ = true;
    render_thread_ = std::thread(&render::render_thread_main, this);
    return true;
}

void cots::graphics::render::deinitialize() noexcept
{
    if (!running_.exchange(false)) return;

    if (render_thread_.joinable())
    {
        if (!render_ready_.load(std::memory_order_acquire))
        {
            spdlog::info("[render] waiting for render thread to finish init (texture bake may be in progress)");
        }
        render_thread_.join();
    }

    unsubscribe_events();
    spdlog::info(("Renderer deinitialized"));
}

void cots::graphics::render::begin_update(const float dt)
{
    if (!render_ready_.load(std::memory_order_acquire)) return;

    //~ publish what the game built last frame into pending
    auto& building = snapshots_.next_build();
    building.frame_id   = snapshots_.frame_counter++;
    building.delta_time = dt;

    publish_snapshot();
}

void cots::graphics::render::end_update()
{
}

cots::graphics::scene_snapshot & cots::graphics::render::building_snapshot() noexcept
{
    return snapshots_.next_build();
}

cots::graphics::hardware::swapchain& cots::graphics::render::swapchain() noexcept
{
    return swapchain_;
}

const cots::graphics::hardware::swapchain & cots::graphics::render::swapchain() const noexcept
{
    return swapchain_;
}

void cots::graphics::render::render_thread_main()
{
    if (not initialize_render_thread())
    {
        running_ = false;
        spdlog::error("render thread init failed");
        return;
    }

    spdlog::info("render thread started");
    SetThreadDescription(GetCurrentThread(), L"Cots Renderer");
    COTS_PROFILE_THREAD_NAME("Render");
    frame_.start_time_ = std::chrono::steady_clock::now();

    while (running_.load(std::memory_order_relaxed))
    {
        process_pending_commands();
        //~ occlusion check
        if (not swapchain_.check_occlusion())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        const bool had_new = acquire_snapshot();
        draw_frame(snapshots_.latest());

        COTS_PROFILE_FRAME_MARK("Render");
    }

    if (not fence_.wait(fence_.last_signaled_value())) [[unlikely]]
    {
        spdlog::error("[very unexpected] fence wait failed");
    }

#if COTS_EDITOR_ENABLED
    editor::shutdown_on_render_thread();
#endif

    //~ destroy resources
    graph_         .clear();
    shader_cache_  .deinitialize();
    texture_cache_ .deinitialize();
    mesh_cache_    .deinitialize();
    mesh_registry_ .deinitialize();
    textures_      .deinitialize();
    bindless_heap_ .deinitialize();
    buffers_       .deinitialize();
    depth_target_  .deinitialize();
    fence_         .deinitialize();
    swapchain_     .deinitialize();
    device_        .deinitialize();

    spdlog::info("render thread stopped");
}

bool cots::graphics::render::initialize_render_thread()
{
    COTS_PROFILE_SCOPE("render::initialize_render_thread");
    //~ initialize device
    if (not device_.initialize())
    {
        spdlog::error("device init failed");
        return false;
    }

    if (not buffers_.initialize(device_))
    {
        spdlog::error("buffer manager init failed");
        return false;
    }

    //~ bindless heap capacity
    constexpr std::uint32_t bindless_capacity = 4096u;
    if (not bindless_heap_.initialize(device_, bindless_capacity))
    {
        spdlog::error("bindless heap init failed");
        return false;
    }

    if (not textures_.initialize(device_, bindless_heap_))
    {
        spdlog::error("texture manager init failed");
        return false;
    }

    if (not texture_cache_.initialize())
    {
        spdlog::error("texture cache init failed");
        return false;
    }

    //~ default texture path
    //~ debug live release baked
#if COTS_DEBUG && !COTS_TEXTURES_USE_BAKED_IN_DEBUG
    using_baked_path_ = false;
#else
    using_baked_path_ = true;
#endif
    spdlog::info("[render] texture path on startup: {}",
                 using_baked_path_ ? "baked" : "live");

    if (not mesh_registry_.initialize(buffers_))
    {
        spdlog::error("mesh registry init failed");
        return false;
    }
    //~ testing quad
    {
        //~ unit quad in XY
        static constexpr float positions[] =
        {
            -0.5f, -0.5f, 0.0f,   0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,  -0.5f,  0.5f, 0.0f,
        };
        static constexpr float colors[] =
        {
            1,0,0,  0,1,0,  0,0,1,  1,1,1,
        };
        static constexpr float texcoords[] =
        {
            0.0f, 1.0f,   1.0f, 1.0f,
            1.0f, 0.0f,   0.0f, 0.0f,
        };
        static constexpr std::uint16_t indices[] = { 0,1,2, 0,2,3 };

        meshes::mesh_desc qd{};
        qd.streams =
        {
            { "POSITION", positions, sizeof(positions), sizeof(float) * 3 },
            { "COLOR",    colors,    sizeof(colors),    sizeof(float) * 3 },
            { "TEXCOORD", texcoords, sizeof(texcoords), sizeof(float) * 2 },
        };
        qd.vertex_count = 4;
        qd.index_data   = indices;
        qd.index_bytes  = sizeof(indices);
        qd.index_count  = 6;
        qd.index_16bit  = true;
        qd.debug_name   = "quad";

        const auto id = mesh_registry_.create(qd);
        spdlog::info("[render] registered quad as mesh {}", id);
    }

    //~ testing unit cube
    {
        //~ four vertices per face
        static constexpr float positions[] =
        {
            //~ positive x face right
             0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,
            //~ negative x face left
            -0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,  -0.5f,  0.5f,  0.5f,
            //~ positive y face top
            -0.5f,  0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
            //~ negative y face bottom
            -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
             0.5f, -0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
            //~ positive z face front
             0.5f, -0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
            //~ negative z face back
            -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        };
        static constexpr float colors[] =
        {
            1,0,0, 1,0,0, 1,0,0, 1,0,0,
            0,1,0, 0,1,0, 0,1,0, 0,1,0,
            0,0,1, 0,0,1, 0,0,1, 0,0,1,
            1,1,0, 1,1,0, 1,1,0, 1,1,0,
            1,0,1, 1,0,1, 1,0,1, 1,0,1,
            0,1,1, 0,1,1, 0,1,1, 0,1,1,
        };
        static constexpr float texcoords[] =
        {
            0,1, 1,1, 1,0, 0,0, //~ right
            0,1, 1,1, 1,0, 0,0, //~ left
            0,1, 1,1, 1,0, 0,0, //~ top
            0,1, 1,1, 1,0, 0,0, //~ bottom
            0,1, 1,1, 1,0, 0,0, //~ front
            0,1, 1,1, 1,0, 0,0, //~ back
        };
        static constexpr std::uint16_t indices[] =
        {
            //~ culling is none
             0, 1, 2,    0, 2, 3,
             4, 5, 6,    4, 6, 7,
             8, 9,10,    8,10,11,
            12,13,14,   12,14,15,
            16,17,18,   16,18,19,
            20,21,22,   20,22,23,
        };

        meshes::mesh_desc cd{};
        cd.streams =
        {
            { "POSITION", positions, sizeof(positions), sizeof(float) * 3 },
            { "COLOR",    colors,    sizeof(colors),    sizeof(float) * 3 },
            { "TEXCOORD", texcoords, sizeof(texcoords), sizeof(float) * 2 },
        };
        cd.vertex_count = 24;
        cd.index_data   = indices;
        cd.index_bytes  = sizeof(indices);
        cd.index_count  = 36;
        cd.index_16bit  = true;
        cd.debug_name   = "cube";

        const auto id = mesh_registry_.create(cd);
        spdlog::info("[render] registered cube as mesh {}", id);
    }

     //~ test
    {
        if (not mesh_cache_.initialize(std::make_unique<meshes::gltf_importer>()))
        {
            spdlog::error("mesh cache init failed");
            return false;
        }

        constexpr const char* model_path = "assets/models/test.glb";
        meshes::imported_model im{};
        if (mesh_cache_.get_or_bake(model_path, im) && im.valid())
        {
            const auto id = mesh_registry_.create_from_imported(im, "ship");
            if (id == meshes::invalid_mesh)
                spdlog::error("[render] ship registration failed");
            else
                spdlog::info("[render] registered ship from '{}' as mesh {}",
                             model_path, id);
        }
        else
        {
            //~ cube fallback for the ship slot
            spdlog::warn("[render] '{}' missing using cube as ship fallback",
                         model_path);
            static constexpr float positions[] =
            {
                -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
            };
            static constexpr float colors[] =
            {
                1,1,1, 1,1,1, 1,1,1, 1,1,1,
                1,1,1, 1,1,1, 1,1,1, 1,1,1,
            };
            static constexpr float texcoords[] =
            {
                0,0, 1,0, 1,1, 0,1,
                0,0, 1,0, 1,1, 0,1,
            };
            static constexpr std::uint16_t indices[] =
            {
                0,1,2, 0,2,3,    4,6,5, 4,7,6,
                0,3,7, 0,7,4,    1,5,6, 1,6,2,
                0,4,5, 0,5,1,    3,2,6, 3,6,7,
            };
            meshes::mesh_desc sd{};
            sd.streams = {
                { "POSITION", positions, sizeof(positions), sizeof(float) * 3 },
                { "COLOR",    colors,    sizeof(colors),    sizeof(float) * 3 },
                { "TEXCOORD", texcoords, sizeof(texcoords), sizeof(float) * 2 },
            };
            sd.vertex_count = 8;
            sd.index_data   = indices;
            sd.index_bytes  = sizeof(indices);
            sd.index_count  = 36;
            sd.index_16bit  = true;
            sd.debug_name   = "ship_fallback";
            const auto id = mesh_registry_.create(sd);
            spdlog::info("[render] registered ship fallback as mesh {}", id);
        }
    }

    //~ initialize fence
    if (not fence_.initialize(device_))
    {
        spdlog::error("fence init failed");
        return false;
    }

    //~ initialize swapchain
    const auto windows = feature::locator::resolve<platform::windows>();
    const auto size = windows->get_window_size<std::uint32_t>();

    hardware::swapchain_create_info swapchain_info{};
    swapchain_info.allow_tearing = true;
    swapchain_info.width         = size.width;
    swapchain_info.height        = size.height;
    swapchain_info.mode          = hardware::display_mode::windowed;
    swapchain_info.frame_count   = 3;
    swapchain_info.window_handle = windows->get_window_handle();

    if (not swapchain_.initialize(device_, swapchain_info))
    {
        spdlog::error("swapchain init failed");
        return false;
    }

    if (not depth_target_.initialize(device_, swapchain_.width(), swapchain_.height()))
    {
        spdlog::error("depth target init failed");
        return false;
    }
    //~ initialize contexts
    for (std::uint32_t i = 0; i < hardware::frame_count; ++i)
    {
        if (not frame_.contexts[i].initialize(device_, hardware::command_list_type::direct))
        {
            spdlog::error("command list init failed");
            return false;
        }
    }
    frame_.fence_values.fill(0);
    frame_.index = 0u;
    frame_.submit_lists.reserve(hardware::max_submit_lists);

    //~ json in debug packed binary otherwise
#if COTS_DEBUG
    auto storage = std::make_unique<shaders::json_shader_storage>("compiled/shader_cache.json");
#else
    auto storage = std::make_unique<shaders::binary_shader_storage>("compiled/shader_cache.bin");
#endif
    if (not shader_cache_.initialize(std::move(storage)))
    {
        spdlog::error("shader cache init failed");
        return false;
    }

    //~ test compile
    {
        const auto vs = shader_cache_.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "VSMain",
            shaders::shader_stage::vertex
        );
        const auto ps = shader_cache_.get_or_compile(
            "assets/shaders/triangle.hlsl",
            "PSMain",
            shaders::shader_stage::pixel
        );
        spdlog::info("[shader-test] vs={} bytes, ps={} bytes", vs.size, ps.size);
    }

    if (not create_test_texture()) return false;

#if COTS_EDITOR_ENABLED
    //~ editor lives entirely on this thread
    // bindless heap font slot win32 backend dx12 backend retained panels
    {
        const auto win = feature::locator::resolve<platform::windows>();
        const HWND hwnd = win ? win->get_window_handle() : nullptr;
        if (not editor::init_on_render_thread(device_, bindless_heap_, hwnd))
        {
            spdlog::error("[render] editor render thread init failed");
            return false;
        }
    }
#endif

    if (not build_passes()) return false;

    render_ready_.store(true, std::memory_order_release);
    return true;
}

void cots::graphics::render::draw_frame(const scene_snapshot& snap)
{
    COTS_PROFILE_SCOPE("render::draw_frame");

    using clock = std::chrono::steady_clock;

    //~ TODO: warp this ins frame stats data or something later ig
    static clock::time_point window_start = clock::now();
    static double            accum_ms     = 0.0;
    static std::uint32_t     accum_frames = 0;

    const auto frame_begin = clock::now();
    const std::uint32_t frame = frame_.index;

    {
        COTS_PROFILE_SCOPE("render::draw_frame::wait");
        fence_.wait(frame_.fence_values[frame]);
    }

    frame_.submit_lists.clear();
    {
        COTS_PROFILE_SCOPE("render::draw_frame::record");
        record_frame(frame, snap, frame_.submit_lists);
    }
    {
        COTS_PROFILE_SCOPE("render::draw_frame::submit");
        submit_frame(frame_.submit_lists);
    }

    {
        COTS_PROFILE_SCOPE("render::draw_frame::present");
        swapchain_.present(0);
    }

    frame_.fence_values[frame] = fence_.signal(device_.graphics_queue());
    frame_.step();

    //~ telemetry
    const auto   frame_end = clock::now();
    accum_ms += std::chrono::duration<double, std::milli>(frame_end - frame_begin).count();
    ++accum_frames;

    const double elapsed = std::chrono::duration<double>(frame_end - window_start).count();
    if (elapsed >= 1.0 && accum_frames > 0)
    {
        stat_fps_     .store(static_cast<float>(accum_frames / elapsed),       std::memory_order_relaxed);
        stat_frame_ms_.store(static_cast<float>(accum_ms / accum_frames),      std::memory_order_relaxed);

        window_start = frame_end;
        accum_ms     = 0.0;
        accum_frames = 0;
    }
}

void cots::graphics::render::record_frame(
    const std::uint32_t frame,
    const scene_snapshot& snap,
    std::vector<ID3D12CommandList*>& out)
{
    COTS_PROFILE_SCOPE("render::record_frame");

    auto& ctx = frame_.contexts[frame];
    if (!ctx.reset()) return;

    //~ bind the bindless heap
    ctx.set_descriptor_heap(bindless_heap_.heap());

    const graph::execute_context ec
    {
        .ctx         = ctx,
        .snap        = snap,
        .width       = swapchain_.width(),
        .height      = swapchain_.height(),
        .frame_index = frame,
    };

    graph_.execute(ec);

    if (not ctx.close()) [[unlikely]]
    {
        spdlog::error("command list close failed");
        return;
    }
    out.push_back(ctx.list());
}

void cots::graphics::render::process_pending_commands()
{
    COTS_PROFILE_SCOPE("render::process_pending_commands");

    decltype(pending_)                 cmd;
    std::vector<std::function<void()>> editor_cmds;
    {
        std::lock_guard lock(command_mutex_);
        if (!pending_.any() && pending_editor_commands_.empty()) return;
        cmd      = pending_;
        pending_ = {};
        editor_cmds.swap(pending_editor_commands_);
    }

    //~ drain editor side commands first
    // these are gpu affecting lambdas pushed by the editor on the main thread
    for (auto& fn : editor_cmds)
    {
        if (fn) fn();
    }

    //~ shader ops are CPU-only no GPU flush needed
    if (cmd.shader_save)   shader_cache_.flush();
    if (cmd.shader_clear)  shader_cache_.clear();
    if (cmd.shader_reload) shader_cache_.recompile(cmd.shader_reload_key);

    if (cmd.resize &&
       cmd.resize_w == swapchain_.width() &&
       cmd.resize_h == swapchain_.height())
    {
        cmd.resize = false;
    }
    if (cmd.set_win_size &&
        cmd.win_w == swapchain_.width() &&
        cmd.win_h == swapchain_.height() &&
        swapchain_.current_mode() == hardware::display_mode::windowed)
    {
        cmd.set_win_size = false;
    }

    //~ swapchain ops need the GPU idle first
    if (cmd.change_mode || cmd.set_win_size || cmd.resize)
    {
        const std::uint64_t flush = fence_.signal(device_.graphics_queue());
        if (not fence_.wait(flush))
            spdlog::error("[render] gpu flush before swapchain change failed");

        bool ok = true;
        if (cmd.change_mode)  ok = swapchain_.set_display_mode (device_, cmd.mode) && ok;
        if (cmd.set_win_size) ok = swapchain_.set_windowed_size(device_, cmd.win_w, cmd.win_h) && ok;
        if (cmd.resize)       ok = swapchain_.resize           (device_, cmd.resize_w, cmd.resize_h) && ok;
        if (not ok) spdlog::error("[render] swapchain command(s) failed");

        if (depth_target_.width()  != swapchain_.width() ||
            depth_target_.height() != swapchain_.height())
        {
            if (not depth_target_.resize(device_, swapchain_.width(), swapchain_.height()))
                spdlog::error("[render] depth target resize failed");
        }
        graph_.invalidate_resource_states();
        frame_.fence_values.fill(flush);
        frame_.index = 0u;
    }

    //~ wipe the container
    if (cmd.texture_clear_cache)
    {
        texture_cache_.invalidate("assets/textures/checker.png");
    }

    //~ swap live and baked
    if (cmd.texture_toggle_bake)
    {
        const std::uint64_t flush = fence_.signal(device_.graphics_queue());
        if (not fence_.wait(flush))
            spdlog::error("[render] gpu flush before texture toggle failed");

        textures_.destroy(test_texture_);
        test_texture_ = {};

        using_baked_path_ = !using_baked_path_;
        spdlog::info("[render] texture path now {}",
                     using_baked_path_ ? "baked" : "live");

        if (not create_test_texture())
        {
            spdlog::error("[render] toggle recreate failed");
            return;
        }

        //~ rebuild for the new slot
        if (not build_passes())
        {
            spdlog::error("[render] toggle rebuild failed");
            return;
        }
        frame_.fence_values.fill(flush);
        frame_.index = 0u;
    }
}

bool cots::graphics::render::create_test_texture()
{
    constexpr const char* source_path = "assets/textures/checker.png";

    if (using_baked_path_)
    {
        //~ bake or load from disk
        textures::baked_blob blob{};
        if (texture_cache_.get_or_bake(
                source_path, textures::texture_intent::albedo, blob)
            && blob.valid())
        {
            hardware::dds_create_info di{};
            di.dds_data   = blob.dds.data();
            di.dds_size   = blob.dds.size();
            di.debug_name = "checker_bc";

            test_texture_ = textures_.create_from_dds(di);
            if (test_texture_.valid())
            {
                spdlog::info("[render] baked test texture in bindless slot {}",
                             textures_.bindless_slot(test_texture_));
                return true;
            }
            spdlog::warn("[render] baked path failed falling back to live");
        }
        else
        {
            spdlog::warn("[render] cannot bake '{}' falling back to live", source_path);
        }
        //~ baking failed go live
        using_baked_path_ = false;
    }

    //~ live decode with fallback
    utils::decoded_image img{};
    if (!utils::decode_image_file(source_path, img) || !img.valid())
    {
        spdlog::warn("[render] '{}' missing falling back to procedural checker",
                     source_path);
        utils::make_checkerboard(
            256u, 256u, 32u,
            0xFFFFFFFFu,  //~ white
            0xFF2030E0u,  //~ pirate red
            img);
    }

    hardware::texture_create_info ti{};
    ti.width      = img.width;
    ti.height     = img.height;
    ti.format     = hardware::texture_format::rgba8_unorm_srgb;
    ti.pixels     = img.pixels.data();
    ti.row_pitch  = img.row_pitch();
    ti.debug_name = "checker";

    test_texture_ = textures_.create(ti);
    if (!test_texture_.valid())
    {
        spdlog::error("[render] test texture creation failed");
        return false;
    }
    spdlog::info("[render] live test texture in bindless slot {}",
                 textures_.bindless_slot(test_texture_));
    return true;
}

bool cots::graphics::render::build_passes()
{
    graph_.clear();

    //~ import resources owned by the renderer
    // the backbuffer provider re resolves each frame to the swap chains
    // current frame in flight index so the imported handle always points
    // at the right d3d resource and rtv
    const auto h_backbuffer = graph_.resources().import(
        "backbuffer",
        graph::resource_usage::present,
        [this]() -> graph::resource_view
        {
            return graph::resource_view
            {
                .resource    = swapchain_.current_backbuffer(),
                .view_handle = swapchain_.current_rtv_handle(),
                .width       = swapchain_.width(),
                .height      = swapchain_.height(),
            };
        });

    const auto h_depth = graph_.resources().import(
        "depth",
        graph::resource_usage::common,
        [this]() -> graph::resource_view
        {
            return graph::resource_view
            {
                .resource    = depth_target_.resource(),
                .view_handle = depth_target_.dsv_handle(),
                .width       = depth_target_.width(),
                .height      = depth_target_.height(),
            };
        });

    //~ uploaded in common
    //~ graph transitions on first use
    const auto h_test_texture = graph_.resources().import(
        "test_texture",
        graph::resource_usage::common,
        [this]() -> graph::resource_view
        {
            return graph::resource_view
            {
                .resource    = textures_.resource(test_texture_),
                .view_handle = 0,
                .width       = textures_.width(test_texture_),
                .height      = textures_.height(test_texture_),
            };
        },
        true /* preserve_contents */);

    const std::uint32_t test_index = textures_.bindless_slot(test_texture_);

    //~ insertion order TODO: add automatic topological sort
    graph_.add_pass(std::make_unique<passes::clear_pass>  (h_backbuffer, h_depth));
    graph_.add_pass(std::make_unique<passes::mesh_pass>   (h_backbuffer, h_depth, h_test_texture, test_index));
#if COTS_EDITOR_ENABLED
    //~ editor ui draws over the scene
    graph_.add_pass(editor::make_editor_pass(h_backbuffer));
#endif
    graph_.add_pass(std::make_unique<passes::present_pass>(h_backbuffer));

    const setup_context sc
    {
        .device   = device_,
        .shaders  = shader_cache_,
        .buffers  = buffers_,
        .meshes   = mesh_registry_,
        .textures = textures_,
        .bindless = bindless_heap_,
    };

    if (not graph_.compile(sc))
    {
        spdlog::error("[render] graph compile failed");
        return false;
    }
    return true;
}

void cots::graphics::render::submit_frame(const std::vector<ID3D12CommandList*> &lists) const
{
    COTS_PROFILE_SCOPE("render::submit_frame");

    //~ serial submission
    if (lists.empty()) return;

    device_.graphics_queue()->ExecuteCommandLists(
        static_cast<UINT>(lists.size()),
        lists.data()
    );
}

void cots::graphics::render::subscribe_events()
{
    const auto d = feature::locator::resolve<events::dispatcher>();
    d->subscribe<events::window_resized,                 &render::on_window_resized>       (*this);
    d->subscribe<events::swapchain::set_display_mode,    &render::on_set_display_mode>     (*this);
    d->subscribe<events::swapchain::set_windowed_size,   &render::on_set_windowed_size>    (*this);
    d->subscribe<events::shader::save,                   &render::on_shader_save>          (*this);
    d->subscribe<events::shader::clear,                  &render::on_shader_clear>         (*this);
    d->subscribe<events::shader::reload,                 &render::on_shader_reload>        (*this);
    d->subscribe<events::texture::toggle_bake_path,      &render::on_texture_toggle_bake>  (*this);
    d->subscribe<events::texture::clear_bake_cache,      &render::on_texture_clear_cache>  (*this);
}

void cots::graphics::render::unsubscribe_events()
{
    const auto d = feature::locator::resolve<events::dispatcher>();
    d->unsubscribe<events::window_resized,                 &render::on_window_resized>       (*this);
    d->unsubscribe<events::swapchain::set_display_mode,    &render::on_set_display_mode>     (*this);
    d->unsubscribe<events::swapchain::set_windowed_size,   &render::on_set_windowed_size>    (*this);
    d->unsubscribe<events::shader::save,                   &render::on_shader_save>          (*this);
    d->unsubscribe<events::shader::clear,                  &render::on_shader_clear>         (*this);
    d->unsubscribe<events::shader::reload,                 &render::on_shader_reload>        (*this);
    d->unsubscribe<events::texture::toggle_bake_path,      &render::on_texture_toggle_bake>  (*this);
    d->unsubscribe<events::texture::clear_bake_cache,      &render::on_texture_clear_cache>  (*this);
}

void cots::graphics::render::on_window_resized(const events::window_resized &event)
{
    std::lock_guard lock(command_mutex_);
    pending_.resize   = true;
    pending_.resize_w = event.width;
    pending_.resize_h = event.height;
}

void cots::graphics::render::on_set_display_mode(const events::swapchain::set_display_mode& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.change_mode = true;
    pending_.mode        = event.mode;
}

void cots::graphics::render::on_set_windowed_size(const events::swapchain::set_windowed_size& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.set_win_size = true;
    pending_.win_w = event.width;
    pending_.win_h = event.height;
}

void cots::graphics::render::on_shader_save(const events::shader::save& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_save = true;
}

void cots::graphics::render::on_shader_clear(const events::shader::clear& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_clear = true;
}

void cots::graphics::render::on_shader_reload(const events::shader::reload& event)
{
    std::lock_guard lock(command_mutex_);
    pending_.shader_reload     = true;
    pending_.shader_reload_key = event.key;
}

void cots::graphics::render::on_texture_toggle_bake(const events::texture::toggle_bake_path& event)
{
    (void)event;
    std::lock_guard lock(command_mutex_);
    pending_.texture_toggle_bake = true;
}

void cots::graphics::render::on_texture_clear_cache(const events::texture::clear_bake_cache& event)
{
    (void)event;
    std::lock_guard lock(command_mutex_);
    pending_.texture_clear_cache = true;
}

void cots::graphics::render::publish_snapshot()
{
    const std::uint32_t just_built = snapshots_.building_idx.load(std::memory_order_relaxed);

    //~ hand it to the RT
    snapshots_.pending_idx.store(just_built, std::memory_order_release);

    //~ pick a new building slot thats neither pending nor being rendered
    //  with 3 slots and at most 2 "taken" (pending + render) one is always free
    const std::uint32_t rendering = snapshots_.render_idx;   //~ read is racy but only used as a hint
    std::uint32_t next = (just_built + 1u) % 3u;
    if (next == rendering) next = (next + 1u) % 3u;

    snapshots_.building_idx.store(next, std::memory_order_relaxed);

    //~ clear the fresh building slot for this frames writes
    snapshots_.scene[next].clear();
}

bool cots::graphics::render::acquire_snapshot()
{
    COTS_PROFILE_SCOPE("render::acquire_snapshot");

    const std::uint32_t pending =
        snapshots_.pending_idx.exchange(invalid_idx, std::memory_order_acquire);

    if (pending == invalid_idx)
        return false;   //~ nothing new - caller redraws render_idx_

    snapshots_.render_idx = pending;
    return true;
}

void cots::graphics::render::enqueue_editor_command(std::function<void()> fn)
{
    if (!fn) return;
    std::lock_guard lock(command_mutex_);
    pending_editor_commands_.push_back(std::move(fn));
}

std::uint32_t cots::graphics::render::runtime_load_mesh(
    const std::string& path, const std::string& label)
{
    meshes::imported_model im{};
    if (not mesh_cache_.get_or_bake(path, im) || not im.valid())
    {
        spdlog::warn("[render] runtime mesh '{}' load failed", path);
        return ~0u;
    }

    const std::uint64_t flush = fence_.signal(device_.graphics_queue());
    if (not fence_.wait(flush))
    {
        spdlog::warn("[render] gpu flush before runtime mesh load failed");
    }

    const auto id = mesh_registry_.create_from_imported(
        im, label.empty() ? "runtime_mesh" : label.c_str());
    if (id == meshes::invalid_mesh)
    {
        spdlog::warn("[render] runtime mesh '{}' registration failed", path);
        return ~0u;
    }

    if (not build_passes())
    {
        spdlog::warn("[render] graph rebuild after runtime mesh load failed");
    }
    frame_.fence_values.fill(flush);
    frame_.index = 0u;
    return id;
}

bool cots::graphics::render::runtime_load_texture(const std::string& path)
{
    utils::decoded_image img{};
    if (not utils::decode_image_file(path, img) || not img.valid())
    {
        return false;
    }

    hardware::texture_create_info ti{};
    ti.width      = img.width;
    ti.height     = img.height;
    ti.format     = hardware::texture_format::rgba8_unorm_srgb;
    ti.pixels     = img.pixels.data();
    ti.row_pitch  = img.row_pitch();
    ti.debug_name = "runtime_texture";

    const auto h = textures_.create(ti);
    if (not h.valid())
    {
        return false;
    }
    spdlog::info("[render] runtime texture '{}' in bindless slot {}",
                 path, textures_.bindless_slot(h));
    return true;
}
