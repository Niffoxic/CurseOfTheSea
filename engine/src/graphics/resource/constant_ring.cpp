// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/resource/constant_ring.h"

#include "engine/graphics/hardware/buffer_manager.h"
#include "engine/utils/helpers.h"

#include <spdlog/spdlog.h>

namespace cots::graphics::resource
{
    bool constant_ring::initialize(hardware::buffer_manager& bm,
                                   const std::uint32_t record_size,
                                   const std::uint32_t record_count,
                                   const char* debug_name)
    {
        bm_     = &bm;
        count_  = record_count ? record_count : 1;
        stride_ = static_cast<std::uint32_t>(
            helpers::adjust_to_256(record_size, true));

        for (std::uint32_t f = 0; f < hardware::frame_count; ++f)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes = static_cast<std::uint64_t>(stride_) * count_;
            bi.kind       = hardware::buffer_kind::constant;
            bi.debug_name = debug_name;

            buffers_[f] = bm.create(bi);
            if (!buffers_[f].valid())
            {
                spdlog::error("[constant_ring] alloc failed ({}, frame {})", debug_name, f);
                return false;
            }
        }
        return true;
    }

    void constant_ring::deinitialize()
    {
        if (!bm_) return;

        for (auto& h : buffers_)
        {
            bm_->destroy(h); h = {};
        }
        bm_ = nullptr;
    }

    void* constant_ring::cpu(const std::uint32_t frame, const std::uint32_t i) const
    {
        auto* base = static_cast<std::uint8_t*>(bm_->mapped_ptr(buffers_[frame]));
        return base ? base + static_cast<std::size_t>(i) * stride_ : nullptr;
    }

    std::uint64_t constant_ring::gpu(const std::uint32_t frame, const std::uint32_t i) const
    {
        const std::uint64_t base = bm_->gpu_address(buffers_[frame]);
        return base ? base + static_cast<std::uint64_t>(i) * stride_ : 0;
    }
} // namespace cots::graphics::resource
