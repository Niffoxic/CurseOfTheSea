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
#ifndef CURSEOFTHESEA_LAYER_STACK_H
#define CURSEOFTHESEA_LAYER_STACK_H
#include "gameplay/gameplay_api.h"

#include "gameplay/layer.h"

#include <memory>
#include <vector>

namespace gameplay
{
    class layer_stack
    {
        using container      = std::vector<std::unique_ptr<layer>>;
        using iterator       = container::iterator;
        using const_iterator = container::const_iterator;
    public:
         layer_stack();
        ~layer_stack();

        layer_stack(const layer_stack&) = delete;
        layer_stack(layer_stack&&)      = delete;

        layer_stack& operator=(const layer_stack&) = delete;
        layer_stack& operator=(layer_stack&&)      = delete;

        // On Attach will be called immediately after attaching
        void push_layer  (std::unique_ptr<layer> l);
        void push_overlay(std::unique_ptr<layer> overlay);

        iterator       begin()       noexcept { return layers_.begin(); }
        iterator       end  ()       noexcept { return layers_.end  (); }

        [[nodiscard]] const_iterator begin() const noexcept { return layers_.begin(); }
        [[nodiscard]] const_iterator end  () const noexcept { return layers_.end  (); }

        [[nodiscard]] std::size_t size () const noexcept { return layers_.size (); }
        [[nodiscard]] bool        empty() const noexcept { return layers_.empty(); }

        // detaches every layer in lifo order
        void clear();

    private:
        container      layers_;
        std::ptrdiff_t insert_index_{ 0 };
    };
} // namespace gameplay

#endif //CURSEOFTHESEA_LAYER_STACK_H
