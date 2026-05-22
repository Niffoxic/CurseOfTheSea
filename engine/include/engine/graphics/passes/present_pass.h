// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PRESENT_PASS_H
#define CURSEOFTHESEA_PRESENT_PASS_H

#include "engine/graphics/passes/pass.h"

namespace cots::graphics::passes
{
    //~ terminal node transitions backbuffer to present and resets depth
    class present_pass final : public pass
    {
    public:
        explicit present_pass(graph::resource_handle backbuffer) noexcept;

        void declare(graph::declare_context& dc) override;
        void execute(const pass_context& pc)     override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "present_pass";
        }

    private:
        graph::resource_handle backbuffer_;
    };
} // namespace cots::graphics::passes

#endif //CURSEOFTHESEA_PRESENT_PASS_H
