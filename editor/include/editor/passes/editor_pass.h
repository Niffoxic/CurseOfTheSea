// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_EDITOR_PASS_H
#define CURSEOFTHESEA_EDITOR_PASS_H

#include "engine/graphics/passes/pass.h"

namespace cots::editor
{
    //~ render graph pass that draws the imgui frame onto the backbuffer
    class editor_pass final : public graphics::pass
    {
    public:
        explicit editor_pass(graphics::graph::resource_handle backbuffer) noexcept;

        void declare(graphics::graph::declare_context& dc) override;
        void execute(const graphics::pass_context&    pc) override;

        [[nodiscard]] const char* name() const noexcept override
        {
            return "editor_pass";
        }

    private:
        graphics::graph::resource_handle backbuffer_;
    };
} // namespace cots::editor

#endif //CURSEOFTHESEA_EDITOR_PASS_H
