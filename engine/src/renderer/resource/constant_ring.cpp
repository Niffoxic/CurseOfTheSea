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
#include "trishul/renderer/resource/constant_ring.h"

#include "trishul/renderer/hardware/buffer_manager.h"
#include "trishul/utils/statics.h"
#include "trishul/utils/logger.h"

namespace trishul::render::resource
{
    bool constant_ring::initialize(hardware::buffer_manager& bm,
                                   const std::uint32_t record_size,
                                   const std::uint32_t record_count,
                                   const char*         debug_name)
    {
        //~ re init after a device rebuild so dropping the old handles first
        if (bm_) deinitialize();

        bm_     = &bm;
        count_  = record_count ? record_count : 1u;
        stride_ = static_cast<std::uint32_t>(
            statics::align_cb_size(record_size, true));

        for (std::uint32_t f = 0; f < config::FRAME_COUNT; ++f)
        {
            hardware::buffer_create_info bi{};
            bi.size_bytes = static_cast<std::uint64_t>(stride_) * count_;
            bi.kind       = hardware::buffer_kind::constant;
            bi.debug_name = debug_name;

            buffers_[f] = bm.create(bi);
            if (!buffers_[f].valid())
            {
                LOG_ERROR("constant_ring alloc failed {} frame {}", debug_name, f);
                deinitialize();
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
            if (h.valid()) bm_->destroy(h);
            h = {};
        }
        bm_     = nullptr;
        stride_ = 0u;
        count_  = 0u;
    }

    void* constant_ring::cpu(const std::uint32_t frame, const std::uint32_t i) const
    {
        if (!bm_ || frame >= config::FRAME_COUNT || i >= count_) return nullptr;

        auto* base = static_cast<std::uint8_t*>(bm_->mapped_ptr(buffers_[frame]));
        return base ? base + static_cast<std::size_t>(i) * stride_ : nullptr;
    }

    std::uint64_t constant_ring::gpu(const std::uint32_t frame, const std::uint32_t i) const
    {
        if (!bm_ || frame >= config::FRAME_COUNT || i >= count_) return 0u;

        const std::uint64_t base = bm_->gpu_address(buffers_[frame]);
        return base ? base + static_cast<std::uint64_t>(i) * stride_ : 0u;
    }
} // namespace trishul::render::resource
