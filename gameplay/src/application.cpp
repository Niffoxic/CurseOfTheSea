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
#include "gameplay/application.h"
#include "gameplay/layer_stack.h"

#include <trishul/engine.h>
#include <trishul/core/engine_assert.h>
#include <trishul/core/engine_config.h>
#include <timeapi.h>

namespace gameplay
{
    struct application::impl
    {
        explicit impl(trishul::engine_create_info info)
        : engine_(std::move(info))
        {}

        trishul::engine engine_;
        layer_stack     stack_   {};
        bool            attached_{ false };
    };

    application::application(trishul::engine_create_info info)
    : p_(std::make_unique<impl>(std::move(info)))
    {}

    application::~application() = default;

    void application::push_layer(std::unique_ptr<layer> l)
    {
        if (not l) return;
        l->set_application(this);

        if (p_->attached_) l->on_attach();
        p_->stack_.push_layer(std::move(l));
    }

    void application::push_overlay(std::unique_ptr<layer> l)
    {
        if (not l) return;
        l->set_application(this);

        if (p_->attached_) l->on_attach();
        p_->stack_.push_overlay(std::move(l));
    }

    int application::run() const
    {
#if defined(COTS_DEBUG_RUNTIME)
        trishul::init_debug_runtime();
#endif

        timeBeginPeriod(1);

        if (not p_->engine_.initialize())
        {
            timeEndPeriod(1);
            return 1;
        }

        while (not p_->engine_.should_close())
        {
            p_->engine_.tick();

            if (not p_->attached_)
            {
                for (auto& l : p_->stack_)
                {
                    if (l) l->on_attach();
                }
                p_->attached_ = true;
            }

            if (p_->attached_)
            {
                const float dt = p_->engine_.delta_time();
                for (auto& l : p_->stack_) if (l) l->on_update(dt);
                for (auto& l : p_->stack_) if (l) l->on_render();
            }
        }
        p_->stack_.clear();
        timeEndPeriod(1);
        return 0;
    }
} // namespace gameplay
