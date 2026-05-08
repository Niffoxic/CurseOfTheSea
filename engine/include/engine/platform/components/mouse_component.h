// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MOUSE_COMPONENT_H
#define CURSEOFTHESEA_MOUSE_COMPONENT_H

#include <bitset>
#include <vector>

#include "engine/core/framework/interface/interface_input_component.h"

namespace cots::platform
{
    enum class mouse_button : std::uint8_t
    {
        left   = 0,
        right  = 1,
        middle = 2,
        x1     = 3,   // back or forward side buttons
        x2     = 4,
        count  = 5
    };

    struct mouse_point { int x = 0; int y = 0; };
    struct mouse_delta { int x = 0; int y = 0; };

    class mouse_component final : public interface::interface_input_component
    {
    public:
        static constexpr std::size_t button_count =
            static_cast<std::size_t>(mouse_button::count);

         mouse_component()          = default;
        ~mouse_component() override = default;

        mouse_component(const mouse_component&) = delete;
        mouse_component(mouse_component&&)      = default;

        mouse_component& operator=(const mouse_component&) = delete;
        mouse_component& operator=(mouse_component&&)      = default;

        //~ lifecycle
        [[nodiscard]] bool initialize(const interface::input_initialize_info& info) override;

        void deinitialize() override;
        void begin_update() override;
        void end_update  () override;

        [[nodiscard]] bool poll_messages(
            UINT message, WPARAM w_param,
            LPARAM l_param
        ) override;

        //~ continuous state
        [[nodiscard]] bool is_down(mouse_button b) const noexcept;
        [[nodiscard]] bool is_up  (mouse_button b) const noexcept;

        //~ one-frame edges
        [[nodiscard]] bool pressed       (mouse_button b) const noexcept;
        [[nodiscard]] bool released      (mouse_button b) const noexcept;
        [[nodiscard]] bool double_clicked(mouse_button b) const noexcept;

        //~ position & motion
        [[nodiscard]] mouse_point position()  const noexcept { return position_; }
        [[nodiscard]] mouse_delta raw_delta() const noexcept { return raw_delta_; }

        //~ wheel
        [[nodiscard]] float wheel           () const noexcept { return wheel_;   }
        [[nodiscard]] float wheel_horizontal() const noexcept { return wheel_h_; }

        //~ cursor visibility
        void show_cursor();
        void hide_cursor();

        [[nodiscard]] bool cursor_visible() const noexcept
        {
            return cursor_visible_;
        }

        //~ cursor confinement
        void lock_to_window  () const; // clip cursor to client rect
        void unlock          () const; // release clip
        void center_in_window() const; // move cursor to client center
        void clear           () noexcept;

        [[nodiscard]] static constexpr std::size_t idx(mouse_button b) noexcept
        {
            return static_cast<std::size_t>(b);
        }

        [[nodiscard]] static constexpr bool valid(mouse_button b) noexcept
        {
            return idx(b) < button_count;
        }

        void on_button_down  (mouse_button b) noexcept;
        void on_button_up    (mouse_button b) noexcept;
        void on_double_click (mouse_button b) noexcept;
        void handle_raw_input(LPARAM l_param);

    private:
        std::bitset<button_count> down_          {};
        std::bitset<button_count> pressed_       {};
        std::bitset<button_count> released_      {};
        std::bitset<button_count> double_clicked_{};

        mouse_point position_ {};
        mouse_delta raw_delta_{};

        float wheel_  {   0.0f };
        float wheel_h_{ 0.0f };

        bool              cursor_visible_{ true };
        HWND              hwnd_          { nullptr };
        std::vector<BYTE> raw_buffer_    {};
    };
} // namespace cots::platform

#endif //CURSEOFTHESEA_MOUSE_COMPONENT_H
