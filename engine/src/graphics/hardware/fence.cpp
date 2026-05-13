// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/utils/exception.h"

#include <d3d12.h>
#include <spdlog/spdlog.h>

cots::graphics::hardware::fence::~fence()
{
    deinitialize();
}

bool cots::graphics::hardware::fence::initialize(
    const device& dev, std::uint64_t initial_value)
{
    if (fence_) return true;

    auto* d3d_device = dev.d3d12_device();
    if (!d3d_device)
    {
        spdlog::error("device not initialized");
        return false;
    }

    try
    {
        COTS_DX_THROW_IF_FAILED_MSG(
            d3d_device->CreateFence(
                initial_value,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&fence_)),
            "CreateFence");
    }
    catch (const exception& e)
    {
        spdlog::error("init failed: {}", e.what());
        return false;
    }

    event_ = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event_)
    {
        spdlog::error("CreateEvent failed (gle={})", GetLastError());
        fence_.Reset();
        return false;
    }

    last_signaled_ = initial_value;
    if (not fence_->SetName(L"COTS Fence"))
    {
        spdlog::error("SetName failed - very rare case");
    }
    return true;
}

void cots::graphics::hardware::fence::deinitialize() noexcept
{
    if (event_)
    {
        ::CloseHandle(event_);
        event_ = nullptr;
    }
    fence_.Reset();
    last_signaled_ = 0;
}

std::uint64_t cots::graphics::hardware::fence::signal(ID3D12CommandQueue *queue)
{
    const std::uint64_t target = ++last_signaled_;
    if (queue && fence_)
    {
        if (const HRESULT hr = queue->Signal(fence_.Get(), target); FAILED(hr))
        {
            spdlog::error("Signal failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(hr));
            --last_signaled_;
        }
    }
    return target;
}

bool cots::graphics::hardware::fence::wait(
    const std::uint64_t value,
    const std::uint32_t timeout_ms) const
{
    if (!fence_) return false;
    if (fence_->GetCompletedValue() >= value) return true;

    if (const HRESULT hr = fence_->SetEventOnCompletion(value, event_); FAILED(hr))
    {
        spdlog::error("SetEventOnCompletion failed (hr=0x{:08X})",
                      static_cast<std::uint32_t>(hr));
        return false;
    }

    const DWORD result = ::WaitForSingleObject(event_, timeout_ms);
    return result == WAIT_OBJECT_0;
}

bool cots::graphics::hardware::fence::is_complete(const std::uint64_t value) const
{
    return fence_ && fence_->GetCompletedValue() >= value;
}

std::uint64_t cots::graphics::hardware::fence::completed_value() const
{
    return fence_ ? fence_->GetCompletedValue(): 0;
}

ID3D12Fence1* cots::graphics::hardware::fence::d3d12_fence() const noexcept
{
    return fence_.Get();
}
