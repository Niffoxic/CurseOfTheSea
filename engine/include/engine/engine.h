// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <memory>

namespace cots
{
    namespace platform  { class windows; }
    namespace utils     { class timer;   }

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
        std::shared_ptr<utils::timer>      timer_   { nullptr };
        std::shared_ptr<platform::windows> windows_ { nullptr };
    };
}

#endif //CURSEOFTHESEA_engine_H
