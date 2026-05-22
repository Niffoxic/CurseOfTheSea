// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RENDER_H
#define CURSEOFTHESEA_RENDER_H

#include <mutex>
#include <thread>
#include <array>
#include <vector>
#include <chrono>
#include <atomic>
#include <memory>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"

#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/hardware/command_context.h"
#include "engine/graphics/shaders/shader_cache.h"
#include "engine/graphics/hardware/buffer_manager.h"

#include "engine/events/windows_event.h"
#include "engine/events/graphics_event.h"
#include "hardware/device.h"
#include "hardware/fence.h"
#include "hardware/types.h"

#include "passes/pass.h"
#include "render_snapshot.h"
#include "meshes/mesh_registry.h"

namespace cots::graphics
{
    class render final:
        public interfaces::subsystem,
        public interfaces::tickable
    {
    public:
         render() = default;
        ~render() override;

        render(const render&) = delete;
        render(render&&)      = delete;

        render& operator=(const render&) = delete;
        render& operator=(render&&)      = delete;

        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        //~ main thread
        void begin_update(float dt) override;
        void end_update() override;

        // called by render_services(MT) installer during host update
        [[nodiscard]] scene_snapshot& building_snapshot() noexcept;

        //~ for editor later
        [[nodiscard]] shaders::shader_cache& shader_cache() noexcept
        {
            return shader_cache_;
        }

        [[nodiscard]] hardware::buffer_manager& buffers() noexcept
        {
            return buffers_;
        }

        //~ for tests only
              hardware::swapchain& swapchain() noexcept;
        const hardware::swapchain& swapchain() const noexcept;

        //~ thread safe fps
        [[nodiscard]] float fps     () const noexcept
        {
            return stat_fps_.load(std::memory_order_relaxed);
        }
        //~ thread safe ms
        [[nodiscard]] float frame_ms() const noexcept
        {
            return stat_frame_ms_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] bool is_ready() const noexcept
        {
            return render_ready_.load(std::memory_order_acquire);
        }

    private:
        void render_thread_main      ();
        bool initialize_render_thread();

        void draw_frame  (const scene_snapshot& snap);
        void record_frame(std::uint32_t frame,  const scene_snapshot& snap, std::vector<ID3D12CommandList*>& out);
        void submit_frame(const std::vector<ID3D12CommandList*>& lists) const;

        //~ core
        void process_pending_commands();
        bool build_passes();

        //~ handle events
        void subscribe_events  ();
        void unsubscribe_events();

        void on_window_resized   (const events::window_resized& event);
        void on_set_display_mode (const events::swapchain::set_display_mode& event);
        void on_set_windowed_size(const events::swapchain::set_windowed_size& event);

        void on_shader_save  (const events::shader::save&   event);
        void on_shader_clear (const events::shader::clear&  event);
        void on_shader_reload(const events::shader::reload& event);

        //~ snapshots
        void publish_snapshot(); // on begin update(MT)
        bool acquire_snapshot(); // grabs pending if any(RT)
    private:
        std::thread       render_thread_{};
        std::mutex        command_mutex_;
        std::atomic<bool> running_      { false };
        std::atomic<bool> render_ready_ { false };

        //~ systems
        hardware::device    device_   {};
        hardware::fence     fence_    {};
        hardware::swapchain swapchain_{};

        //~ per frame in flight recording resources
        struct
        {
            std::array<hardware::command_context, hardware::frame_count> contexts    {};
            std::array<std::uint64_t,             hardware::frame_count> fence_values{};
            std::uint32_t index{ 0u };

            std::vector<ID3D12CommandList*>       submit_lists{};
            std::chrono::steady_clock::time_point start_time_ {}; //~ animated clear

            void step()
            {
                index = (index + 1u) % hardware::frame_count;
            }
        } frame_;

        //~ pending commands
        struct
        {
            //~ swapchain events
            bool          resize       { false };
            std::uint32_t resize_w     { 0 };
            std::uint32_t resize_h     { 0 };

            bool          change_mode  { false };
            hardware::display_mode mode { hardware::display_mode::windowed };

            bool          set_win_size { false };
            std::uint32_t win_w        { 0 };
            std::uint32_t win_h        { 0 };

            //~ shader events
            bool          shader_save    { false };
            bool          shader_clear   { false };
            bool          shader_reload  { false };
            std::uint64_t shader_reload_key { 0 };

            [[nodiscard]] bool any() const noexcept
            {
                return resize || change_mode || set_win_size
                  || shader_save || shader_clear || shader_reload;
            }
        } pending_{};

        //~ triple buffered snapshots (TODO: profile this sht)
        static constexpr std::uint32_t invalid_idx = ~0u;
        struct
        {
            std::array<scene_snapshot, 3> scene{};
            std::atomic<std::uint32_t>    building_idx{ 0 };
            std::atomic<std::uint32_t>    pending_idx { invalid_idx };
            std::uint32_t                 render_idx  { 0 }; //~ RTs current slot

            std::uint64_t frame_counter{ 0 };   //~ MT snapshot id source

            scene_snapshot& latest()
            {
                return scene[render_idx];
            }

            scene_snapshot& next_build()
            {
                return scene[building_idx.load(std::memory_order_relaxed)];
            }
        } snapshots_;

        //~ render graph
        std::vector<std::unique_ptr<pass>> passes_;

        //~ benchmarks
        std::atomic<float> stat_fps_     { 0.f };
        std::atomic<float> stat_frame_ms_{ 0.f };

        //~ core
        shaders::shader_cache    shader_cache_{};
        hardware::buffer_manager buffers_     {};
        meshes::mesh_registry   mesh_registry_{};
    };
}

#endif //CURSEOFTHESEA_RENDER_H
