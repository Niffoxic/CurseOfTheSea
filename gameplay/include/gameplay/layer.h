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
#ifndef CURSEOFTHESEA_LAYER_H
#define CURSEOFTHESEA_LAYER_H

#include "gameplay/gameplay_api.h"

#include <string>
#include <string_view>

namespace gameplay
{
    class application;

    class layer
    {
    public:
        explicit layer(std::string name);
        virtual ~layer() = default;

        layer(const layer&) = delete;
        layer(layer&&)      = delete;

        layer& operator=(const layer&)  = delete;
        layer& operator=(layer&&)       = delete;

        virtual void on_attach() {}
        virtual void on_detach() {}
        virtual void on_render() {}
        virtual void on_event () {}

        virtual void on_update(float dt) {}

        //~ getters
        [[nodiscard]] std::string_view  name() const noexcept { return name_; }

        [[nodiscard]]       application* app()       noexcept { return app_; }
        [[nodiscard]] const application* app() const noexcept { return app_; }

        //~ setters
        void set_application(application* app) noexcept { app_ = app; }
    private:
        std::string name_{ "Default Layer" };
        application* app_{ nullptr };
    };
} // namespace gameplay

#endif //CURSEOFTHESEA_LAYER_H
