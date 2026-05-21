// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RENDER_H
#define CURSEOFTHESEA_RENDER_H

#include <mutex>
#include <thread>

#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"

#include "engine/graphics/hardware/swapchain.h"

#include "engine/events/windows_event.h"
#include "hardware/device.h"
#include "hardware/fence.h"

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
        void record_frame       ();
        void submit_frame       ();
        void draw_frame         ();

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

        //~ handle events TODO: Create a command dispatcher instead
        events::window_resized event_pending_resize_{};
        bool has_pending_resize_ { false };
    };
}

#endif //CURSEOFTHESEA_RENDER_H
