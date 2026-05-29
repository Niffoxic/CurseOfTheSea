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
#ifndef CURSEOFTHESEA_APPLICATION_H
#define CURSEOFTHESEA_APPLICATION_H

#include "gameplay/gameplay_api.h"
#include "gameplay/layer.h"

#include <trishul/engine.h>

#include <memory>

namespace gameplay
{
    class application
    {
    public:
        explicit application(trishul::engine_create_info info = {});
        ~application();

        application(const application&) = delete;
        application(application&&)      = delete;

        application& operator=(const application&) = delete;
        application& operator=(application&&)      = delete;

        void push_layer  (std::unique_ptr<layer> l);
        void push_overlay(std::unique_ptr<layer> l);

        int run() const;
    private:
        struct impl;
        std::unique_ptr<impl> p_;
    };
} // namespace gameplay

#endif //CURSEOFTHESEA_APPLICATION_H
