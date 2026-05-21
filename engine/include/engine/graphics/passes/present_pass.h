// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PRESENT_PASS_H
#define CURSEOFTHESEA_PRESENT_PASS_H

#include "engine/graphics/passes/pass.h"

namespace cots::graphics::passes
{
    //~ terminal node
    class present_pass final : public pass
    {
    public:
        void execute(const pass_context& pc) override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "present_pass";
        }
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_PRESENT_PASS_H
