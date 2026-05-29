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
#include "gameplay/layer_stack.h"
#include <ranges>

namespace gameplay
{
    layer_stack::layer_stack() = default;

    layer_stack::~layer_stack()
    {
        clear();
    }

    void layer_stack::push_layer(std::unique_ptr<layer> l)
    {
        if (not l) return;
        layers_.insert(layers_.begin() + insert_index_, std::move(l));
        ++insert_index_;
    }

    void layer_stack::push_overlay(std::unique_ptr<layer> overlay)
    {
        if (not overlay) return;
        layers_.push_back(std::move(overlay));
    }

    void layer_stack::clear()
    {
        for (auto& layer: std::views::reverse(layers_))
        {
            if (layer) layer->on_detach();
        }
        layers_.clear();
        insert_index_ = 0;
    }
} // namespace gameplay
