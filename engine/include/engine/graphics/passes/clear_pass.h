// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_CLEAR_PASS_H
#define CURSEOFTHESEA_CLEAR_PASS_H

#include "engine/graphics/passes/pass.h"

namespace cots::graphics::passes
{
    //~ acquires the backbuffer as a render target and clears it
    //  also flips the depth target into depth_write and clears it
    class clear_pass final : public pass
    {
    public:
        clear_pass(graph::resource_handle backbuffer,
                   graph::resource_handle depth) noexcept;

        void declare(graph::declare_context& dc) override;
        void execute(const pass_context& pc)     override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "clear_pass";
        }

    private:
        graph::resource_handle backbuffer_;
        graph::resource_handle depth_;
    };
}

#endif //CURSEOFTHESEA_CLEAR_PASS_H
