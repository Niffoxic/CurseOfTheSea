// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <memory>
#include <engine/cots_api.h>

namespace cots
{
    class timer;
    class COTS_API engine
    {
    public:
         engine();
        ~engine();

        [[nodiscard]] bool init   ();
        [[nodiscard]] int  execute();

    private:
        std::shared_ptr<timer> timer_{ nullptr };
    };
}

#endif //CURSEOFTHESEA_engine_H
