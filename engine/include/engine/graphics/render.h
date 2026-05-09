// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RENDER_H
#define CURSEOFTHESEA_RENDER_H

#include <thread>

#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/framework/interface/tickable.h"

namespace cots::graphics
{
    class render final:
        public interface::subsystem,
        public interface::tickable
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

    private:
        void render_thread_main ();
        void begin_frame        ();
        void end_frame          ();

    private:
        std::thread       render_thread_{};
        std::atomic<bool> running_{ false };
    };
}

#endif //CURSEOFTHESEA_RENDER_H
