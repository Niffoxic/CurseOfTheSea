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

#include <timeapi.h>

namespace gameplay
{
    struct application::impl
    {
        layer_stack  stack_    {};
        bool         attached_ { false };
    };

    application::application()
    : p_(std::make_unique<impl>())
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

    int application::run()
    {
        return 0;
    }
} // namespace gameplay
