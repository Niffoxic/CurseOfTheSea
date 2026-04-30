// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <engine/cots_api.h>
#include "engine/utils/timer.h"

namespace cots
{
    class COTS_API engine
    {
    public:
         engine();
        ~engine();

        [[nodiscard]] bool init   ();
        [[nodiscard]] int  execute();

    private:
        timer timer_{};
    };
}

#endif //CURSEOFTHESEA_engine_H
