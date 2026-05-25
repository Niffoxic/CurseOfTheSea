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
#ifndef CURSEOFTHESEA_engine_H
#define CURSEOFTHESEA_engine_H

#include <memory>

namespace cots
{
    struct fps_stats
    {
        std::uint32_t main_thread  {};
        std::uint32_t render_thread{};
    };

    class engine
    {
    public:
         engine();
        ~engine();

        engine(const engine&) = delete;
        engine(engine &&)     = delete;

        engine &operator=(const engine &) = delete;
        engine &operator=(engine &&)      = delete;

        [[nodiscard]]
        bool initialize() const;
        void update    ();

        [[nodiscard]] bool  should_close() const noexcept;
        [[nodiscard]] float delta_time  () const noexcept;

        [[nodiscard]] fps_stats get_fps_stats() const noexcept;

        //~ fps setters
        void set_target_fps(std::uint32_t target_fps) const;
    private:
        class implementation;
        std::unique_ptr<implementation> impl_;
    };
} // namespace cots

#endif //CURSEOFTHESEA_engine_H
