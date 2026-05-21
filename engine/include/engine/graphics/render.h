// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RENDER_H
#define CURSEOFTHESEA_RENDER_H

#include <mutex>
#include <thread>
#include <array>
#include <d3d12.h>

#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"

#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/hardware/command_context.h"

#include "engine/events/windows_event.h"
#include "hardware/device.h"
#include "hardware/fence.h"
#include "hardware/types.h"

#include <d3d12.h>
#include <dxgi1_6.h>

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

        //~ for tests only
        hardware::device&    device   () noexcept;
        hardware::swapchain& swapchain() noexcept;
        hardware::fence&     fence    () noexcept;

        const hardware::device&    device   () const noexcept;
        const hardware::swapchain& swapchain() const noexcept;
        const hardware::fence&     fence    () const noexcept;

    private:
        void render_thread_main ();
        void draw_frame         ();

        void record_frame(std::uint32_t frame, std::vector<ID3D12CommandList*>& out);
        void submit_frame(const std::vector<ID3D12CommandList*>& lists) const;

        //~ core
        void process_pending_commands();

        //~ handle events
        void subscribe_events  ();
        void unsubscribe_events();

        void on_window_resized(const events::window_resized& event);

    private:
        std::thread       render_thread_{};
        std::mutex        command_mutex_;
        std::atomic<bool> running_{ false };

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

        //~ handle events TODO: Create a command dispatcher instead
        events::window_resized event_pending_resize_{};
        bool has_pending_resize_ { false };
    };
}

#endif //CURSEOFTHESEA_RENDER_H
