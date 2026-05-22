// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_CONSTANT_RING_H
#define CURSEOFTHESEA_CONSTANT_RING_H

#include <array>
#include <cstdint>

#include "engine/graphics/hardware/types.h"
#include "engine/graphics/hardware/resource.h"

namespace cots::graphics::hardware { class buffer_manager; }

namespace cots::graphics::resource
{
    // persistently mapped upload buffers
    class constant_ring final
    {
    public:
        [[nodiscard]] bool initialize(hardware::buffer_manager& bm,
                                      std::uint32_t record_size,
                                      std::uint32_t record_count,
                                      const char* debug_name);
        void deinitialize();

        [[nodiscard]] void*         cpu(std::uint32_t frame, std::uint32_t i) const;
        [[nodiscard]] std::uint64_t gpu(std::uint32_t frame, std::uint32_t i) const;

        [[nodiscard]] std::uint32_t stride() const noexcept { return stride_; }
        [[nodiscard]] std::uint32_t count () const noexcept { return count_;  }

    private:
        std::array<hardware::buffer_handle, hardware::frame_count> buffers_{};
        hardware::buffer_manager* bm_     { nullptr };
        std::uint32_t             stride_ { 0 };
        std::uint32_t             count_  { 0 };
    };
} // namespace cots::graphics::resource

#endif //CURSEOFTHESEA_CONSTANT_RING_H
