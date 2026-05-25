// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/utils/exception.h"
#include "engine/utils/logger.h"

#include <d3d12.h>

#include "engine/engine.h"
#include "engine/system/define_features.h"

using namespace cots::graphics::hardware;

class fence::implementation
{
public:
     implementation() = default;
    ~implementation();

    implementation(const implementation&) = delete;
    implementation(implementation&&)      = delete;

    implementation& operator=(const implementation&) = delete;
    implementation& operator=(implementation&&)      = delete;

    [[nodiscard]] bool initialize  (const device& dev, std::uint64_t initial_value);
                  void deinitialize() noexcept;

    [[nodiscard]] std::uint64_t signal(ID3D12CommandQueue* queue);

    [[nodiscard]] bool wait(
        std::uint64_t value,
        std::uint32_t timeout_ms
    ) const;

    [[nodiscard]] bool is_complete(std::uint64_t value) const;

    [[nodiscard]] std::uint64_t completed_value    () const;
    [[nodiscard]] std::uint64_t last_signaled_value() const noexcept;
    [[nodiscard]] ID3D12Fence1* d3d12_fence        () const noexcept;

private:
    Microsoft::WRL::ComPtr<ID3D12Fence1> fence_;
    HANDLE event_ = nullptr;

    std::uint64_t last_signaled_ = 0;
};

#pragma region FENCE_MAIN

fence::fence()
    : impl_(std::make_unique<implementation>())
{}

fence::~fence()
{
    impl_->deinitialize();
}

bool fence::initialize(const device& dev, const std::uint64_t initial_value) const
{
    return impl_->initialize(dev, initial_value);
}

void fence::deinitialize() const noexcept
{
    impl_->deinitialize();
}

std::uint64_t fence::signal(ID3D12CommandQueue* queue) const
{
    return impl_->signal(queue);
}

bool fence::wait(const std::uint64_t value, const std::uint32_t timeout_ms) const
{
    return impl_->wait(value, timeout_ms);
}

bool fence::is_complete(const std::uint64_t value) const
{
    return impl_->is_complete(value);
}

std::uint64_t fence::completed_value() const
{
    return impl_->completed_value();
}

std::uint64_t fence::last_signaled_value() const noexcept
{
    return impl_->last_signaled_value();
}

ID3D12Fence1* fence::d3d12_fence() const noexcept
{
    return impl_->d3d12_fence();
}

#pragma endregion

#pragma region FENCE_IMPLEMENTATION

fence::implementation::~implementation()
{
    deinitialize();
}

bool fence::implementation::initialize(
    const device& dev,
    const std::uint64_t initial_value)
{
    if (fence_)
        return true;

    auto* d3d_device = dev.d3d12_device();
    if (!d3d_device)
    {
        LOG_ERROR("Device not initialized");
        return false;
    }

    try
    {
        COTS_DX_THROW_IF_FAILED_MSG(
            d3d_device->CreateFence(
                initial_value,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&fence_)),
            "CreateFence"
        );
    }
    catch (const exception& e)
    {
        LOG_ERROR("Fence init failed: {}", e.what());
        return false;
    }

    event_ = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event_)
    {
        LOG_ERROR("CreateEvent failed gle={}", GetLastError());
        fence_.Reset();
        return false;
    }

    last_signaled_ = initial_value;

    if (FAILED(fence_->SetName(L"COTS Fence")))
    {
        LOG_ERROR("Fence SetName failed");
    }

    return true;
}

void fence::implementation::deinitialize() noexcept
{
    if (event_)
    {
        ::CloseHandle(event_);
        event_ = nullptr;
    }

    fence_.Reset();
    last_signaled_ = 0;
}

std::uint64_t fence::implementation::signal(ID3D12CommandQueue* queue)
{
    const std::uint64_t target = ++last_signaled_;

    if (!queue)
    {
        LOG_ERROR("Fence signal failed queue is null");
        --last_signaled_;
        return last_signaled_;
    }

    if (!fence_)
    {
        LOG_ERROR("Fence signal failed fence is null");
        --last_signaled_;
        return last_signaled_;
    }

    const HRESULT hr = queue->Signal(fence_.Get(), target);
    if (FAILED(hr))
    {
        LOG_ERROR(
            "Fence Signal failed hr=0x{:08X}",
            static_cast<std::uint32_t>(hr)
        );

        --last_signaled_;
        return last_signaled_;
    }

    return target;
}

bool fence::implementation::wait(
    const std::uint64_t value,
    const std::uint32_t timeout_ms) const
{
    if (!fence_)
        return false;

    if (fence_->GetCompletedValue() >= value)
        return true;

    const HRESULT hr = fence_->SetEventOnCompletion(value, event_);
    if (FAILED(hr))
    {
        LOG_ERROR(
            "SetEventOnCompletion failed hr=0x{:08X}",
            static_cast<std::uint32_t>(hr)
        );

        return false;
    }

    const DWORD result = ::WaitForSingleObject(event_, timeout_ms);
    return result == WAIT_OBJECT_0;
}

bool fence::implementation::is_complete(const std::uint64_t value) const
{
    return fence_ && fence_->GetCompletedValue() >= value;
}

std::uint64_t fence::implementation::completed_value() const
{
    return fence_ ? fence_->GetCompletedValue() : 0;
}

std::uint64_t fence::implementation::last_signaled_value() const noexcept
{
    return last_signaled_;
}

ID3D12Fence1* fence::implementation::d3d12_fence() const noexcept
{
    return fence_.Get();
}

#pragma endregion
