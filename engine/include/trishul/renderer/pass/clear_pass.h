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
#ifndef CURSEOFTHESEA_CLEAR_PASS_H
#define CURSEOFTHESEA_CLEAR_PASS_H

#include "pass.h"
#include "trishul/renderer/clear_color.h"

namespace trishul::render::passes
{
    //~ clears the color target and the reversed z depth target
    class clear_pass final : public pass
    {
    public:
        //~ reversed z so the far plane is zero clear depth to that
        static constexpr float        k_clear_depth   { 0.0f };
        static constexpr std::uint8_t k_clear_stencil { 0u };

        clear_pass(graph::resource_handle color,
                   graph::resource_handle depth) noexcept;

        void declare(graph::declare_context& dc) override;
        void execute(const pass_context& pc)     override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "clear_pass";
        }

    private:
        graph::resource_handle color_;
        graph::resource_handle depth_;
    };
} // namespace trishul::render::passes

#endif //CURSEOFTHESEA_CLEAR_PASS_H
