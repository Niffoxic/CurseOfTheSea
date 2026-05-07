// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <memory>
#include <engine/cots_api.h>

//~ TODO: Upgrade this with Pimp
namespace cots
{
    namespace platform
    {
        class windows;
    }

    namespace utils
    {
        class timer;
    }
    class COTS_API engine
    {
    public:
         engine();
        ~engine();

        [[nodiscard]] bool init   ();
        [[nodiscard]] int  execute();

    private:
        std::shared_ptr<utils::timer> timer_{ nullptr };
        std::shared_ptr<platform::windows> windows_{ nullptr };
    };
}

#endif //CURSEOFTHESEA_engine_H
