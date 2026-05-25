// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_FENCE_H
#define CURSEOFTHESEA_FENCE_H

#include <cstdint>
#include <memory>

struct ID3D12Fence1;
struct ID3D12CommandQueue;

namespace cots::graphics::hardware
{
    class device;

    class fence final
    {
    public:
         fence();
        ~fence();

        fence(const fence&) = delete;
        fence(fence&&)      = delete;

        fence& operator=(const fence&) = delete;
        fence& operator=(fence&&)      = delete;

        [[nodiscard]]
        bool initialize  (const device& dev, std::uint64_t initial_value = 0) const;
        void deinitialize() const noexcept;

        // returns the value that will be reached when GPU completes prior work
        std::uint64_t signal(ID3D12CommandQueue* queue) const;

        // block until fence has reached `value`
        bool wait(std::uint64_t value, std::uint32_t timeout_ms = INT32_MAX) const;

        // non-blocking check
        [[nodiscard]] bool is_complete(std::uint64_t value) const;

        // current GPU-side completed value
        [[nodiscard]] std::uint64_t completed_value() const;

        // last value signaled
        [[nodiscard]] std::uint64_t last_signaled_value() const noexcept;
        [[nodiscard]] ID3D12Fence1* d3d12_fence        () const noexcept;

    private:
        class implementation;
        std::unique_ptr<implementation> impl_;
    };
} // namespace cots::graphics::hardware

#endif

