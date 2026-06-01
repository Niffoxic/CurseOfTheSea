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
#ifndef CURSEOFTHESEA_CONSTANT_RING_H
#define CURSEOFTHESEA_CONSTANT_RING_H

#include <array>
#include <cstdint>

#include "trishul/core/engine_config.h"
#include "trishul/renderer/hardware/resource.h"

namespace trishul::render::hardware { class buffer_manager; }

namespace trishul::render::resource
{
    //~ persistently mapped upload buffers one per frame in flight so the cpu can
    //  scribble next frames constants while the gpu still reads the last one
    class constant_ring final
    {
    public:
        constant_ring() = default;
        ~constant_ring() { deinitialize(); }

        constant_ring           (const constant_ring&) = delete;
        constant_ring& operator=(const constant_ring&) = delete;

        [[nodiscard]] bool initialize(hardware::buffer_manager& bm,
                                      std::uint32_t record_size,
                                      std::uint32_t record_count,
                                      const char*   debug_name);
        void deinitialize();

        //~ cpu write pointer and gpu address for record i on a given frame slot
        //  null or zero when the frame or index is out of range
        [[nodiscard]] void*         cpu(std::uint32_t frame, std::uint32_t i) const;
        [[nodiscard]] std::uint64_t gpu(std::uint32_t frame, std::uint32_t i) const;

        [[nodiscard]] std::uint32_t stride() const noexcept { return stride_; }
        [[nodiscard]] std::uint32_t count () const noexcept { return count_;  }

    private:
        std::array<hardware::buffer_handle, config::FRAME_COUNT> buffers_{};
        hardware::buffer_manager* bm_     { nullptr };
        std::uint32_t             stride_ { 0u };
        std::uint32_t             count_  { 0u };
    };
} // namespace trishul::render::resource

#endif //CURSEOFTHESEA_CONSTANT_RING_H
