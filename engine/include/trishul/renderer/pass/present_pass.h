//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#ifndef CURSEOFTHESEA_PRESENT_PASS_H
#define CURSEOFTHESEA_PRESENT_PASS_H

#include "pass.h"

namespace trishul::render::passes
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
} // namespace trishul::render::passes

#endif //CURSEOFTHESEA_PRESENT_PASS_H
