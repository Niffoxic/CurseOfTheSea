// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <memory>

#include "audio/audio_handle.h"
#include "core/engine_config.h"
#include "core/framework/interface/subsystem.h"
#include "core/framework/interface/tickable.h"
#include "core/framework/dependency_builder.h"

namespace cots::events
{
    struct engine_hit_space {};
}

namespace cots
{
    namespace platform { class windows;     }
    namespace utils    { class timer;       }
    namespace events   { class dispatcher;  }
    namespace audio    { class system;      }
    namespace graphics { class render;      }

    class engine
    {
    public:
         engine();
        ~engine();

        [[nodiscard]] bool init();
                      void tick();

        [[nodiscard]] bool  should_close() const noexcept;
        [[nodiscard]] float delta_time  () const noexcept;

    private:
        void initialize_features();
        void regulate_subsystems();
        void regulate_tickable  ();

        //~ processes
        void update_tickable();

        //~ tests
        void test_debug_input() const;
        void test_fps        () const;
    private:
        config::manager config_manager_{};

        std::shared_ptr<utils::timer>      timer_  { nullptr };
        std::shared_ptr<platform::windows> windows_{ nullptr };
        std::shared_ptr<graphics::render>  render_ { nullptr };

        utils::dependency_scheduler<interfaces::subsystem> subsystem_scheduler_;
        utils::dependency_scheduler<interfaces::tickable>  tickable_scheduler_;
    };
}

#endif //CURSEOFTHESEA_engine_H
