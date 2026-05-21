// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_CLEAR_PASS_H
#define CURSEOFTHESEA_CLEAR_PASS_H

#include "engine/graphics/passes/pass.h"

namespace cots::graphics::passes
{
    //~ TODO: add automatic barrier
    //~ acquires the backbuffer as a render target and clears it
    class clear_pass final : public pass
    {
    public:
        void execute(const pass_context& pc) override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "clear_pass";
        }
    };
}

#endif //CURSEOFTHESEA_CLEAR_PASS_H
