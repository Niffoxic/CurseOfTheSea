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
#ifndef CURSEOFTHESEA_RENDER_H
#define CURSEOFTHESEA_RENDER_H

#include <memory>
#include <thread>

#include "render_snapshot.h"
#include "display.h"
#include "trishul/core/interface/subsystems.h"
#include "trishul/core/interface/tickable.h"

namespace trishul::render
{
    struct graphics_fps
    {
        int fps{};
        int ms {};
    };

    class graphics final
        : public interfaces::subsystems
        , public interfaces::tickable
    {
    public:
         graphics();
        ~graphics() override;

        graphics(const graphics&) noexcept = delete;
        graphics(graphics&&)      noexcept = delete;

        graphics& operator=(const graphics&) noexcept = delete;
        graphics& operator=(graphics&&)      noexcept = delete;

         [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        //~ main thread op
        void begin_update(float dt) override;
        void end_update  ()         override;

        [[nodiscard]] bool is_ready          () const noexcept;
        [[nodiscard]] bool is_bootstrap_ready() const noexcept;

        //~ use this to build snapshots that is renderer going to throw on render pass
        [[nodiscard]] scene_snapshot& building_snapshot() const noexcept;

        //~ helper getters
        [[nodiscard]] graphics_fps frame_fps() const noexcept;

        //~ display settings exposed to the end user so that they can customize it
        [[nodiscard]] display_capabilities display_options() const;

        //~ whatever the device is actually running on right now
        [[nodiscard]] display_settings current_display_settings() const;

        //~ user pick for display changes and applies it
        //~ watch for device_recreated or device_recreate_failed events for the result
        void request_display_settings(const display_settings& settings);

    private:
        //~ render thread entry
        void render_entry() const;

    private:
        struct impl;
        std::unique_ptr<impl> render_;
        std::thread           render_thread_{};
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_RENDER_H
