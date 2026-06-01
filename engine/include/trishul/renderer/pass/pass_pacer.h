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
#ifndef CURSEOFTHESEA_PASS_PACER_H
#define CURSEOFTHESEA_PASS_PACER_H

#include <cstdint>

namespace trishul::render
{
    //~ decides when a throttled pass should fire it banks real frame time and
    //  lets the pass run once enough has piled up for one tick at its target hz
    //  the graph keeps one of these per pass so a 24hz shadow pass skips most
    //  frames while the rest of the renderer keeps screaming along
    class pass_pacer
    {
    public:
        //~ feed it the real frame dt and the passes target hz returns true when
        //~ the pass should run this frame and writes the catch up delta the time
        //~ since it last actually ran into out_dt so stepping stays framerate
        //~ independent hz zero means no throttle run every single frame
        bool should_run(const std::uint32_t hz, const float dt_seconds, float& out_dt) noexcept
        {
            if (hz == 0u) //~ unthrottled hand back the raw frame dt
            {
                out_dt = dt_seconds;
                return true;
            }

            const float period = 1.0f / static_cast<float>(hz);
            accumulated_ += dt_seconds;

            if (accumulated_ + k_epsilon < period) //~ not enough banked yet skip
                return false;

            out_dt = accumulated_;

            //~ keep the leftover so we dont drift but clamp a runaway after a
            //~ hitch or a paused frame so we dont try to replay a hundred ticks
            accumulated_ = (accumulated_ > period * k_max_catch_up)
                ? 0.0f
                : accumulated_ - period;

            return true;
        }

        //~ wipe the banked time eg after a device rebuild or a hard pause
        void reset() noexcept
        {
            accumulated_ = 0.0f;
        }

    private:
        static constexpr float k_epsilon      = 1e-6f;
        static constexpr float k_max_catch_up = 4.0f; //~ periods before we bail

        float accumulated_{ 0.0f };
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_PASS_PACER_H
