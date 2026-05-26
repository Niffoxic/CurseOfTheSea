// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/buffer_manager.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/fence.h"
#include "engine/graphics/hardware/deferred_releaser.h"
#include "engine/graphics/hardware/upload_arena.h"
#include "engine/utils/logger.h"

#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>

#include <cstring>
#include <span>
#include <vector>
#include <cstdint>

using namespace cots::graphics::hardware;

namespace
{
    //~ upload end state
    D3D12_RESOURCE_STATES end_state_for(const buffer_kind kind) noexcept
    {
        switch (kind)
        {
        case buffer_kind::index:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;

        case buffer_kind::skinning_source:
            return D3D12_RESOURCE_STATE_COMMON;

        default:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
    }
}

class buffer_manager::implementation
{
public:
     implementation() = default;
    ~implementation();

    implementation(const implementation&) = delete;
    implementation(implementation&&)      = delete;

    implementation& operator=(const implementation&) = delete;
    implementation& operator=(implementation&&)      = delete;

    [[nodiscard]] bool initialize  (device& dev);
                  void deinitialize() noexcept;

    void set_releaser    (deferred_releaser* releaser) noexcept;
    void set_upload_arena(upload_arena* arena) noexcept;

    [[nodiscard]] buffer_handle create(const buffer_create_info& info);

    [[nodiscard]]
    std::vector<buffer_handle> create_batch(std::span<const buffer_create_info> infos);

    void destroy(buffer_handle handle);

    [[nodiscard]] ID3D12Resource* resource   (buffer_handle handle) const;
    [[nodiscard]] std::uint64_t   gpu_address(buffer_handle handle) const;
    [[nodiscard]] std::uint64_t   size       (buffer_handle handle) const;
    [[nodiscard]] std::uint64_t   stride     (buffer_handle handle) const;
    [[nodiscard]] void*           mapped_ptr (buffer_handle handle) const;

private:
    struct slot
    {
        D3D12MA::Allocation* allocation { nullptr };
        ID3D12Resource*      resource   { nullptr };
        std::uint64_t        size       { 0 };
        std::uint64_t        stride     { 0 };
        void*                mapped     { nullptr };
        std::uint32_t        generation { 0 };
        buffer_kind          kind       { buffer_kind::generic };
    };

    [[nodiscard]] std::uint32_t acquire_slot();

    [[nodiscard]] bool upload_static(
        const slot& target,
        const void* data,
        const std::uint64_t size
    ) const;

    struct allocation_result
    {
        std::uint32_t index { 0 };
        bool          ok    { false };
    };

    [[nodiscard]] allocation_result allocate_only(const buffer_create_info& info);

    struct upload_record
    {
        slot*         dst  { nullptr };
        const void*   data { nullptr };
        std::uint64_t size { 0 };
    };

    [[nodiscard]] bool upload_batch(std::span<const upload_record> records) const;

    void release_slot_inline(slot& target) noexcept;

private:
    device*            device_   { nullptr };
    deferred_releaser* releaser_ { nullptr };
    upload_arena*      arena_    { nullptr };

    std::vector<slot> slots_;

    std::uint32_t next_generation_ { 1 };
};

#pragma region BUFFER_MANAGER_MAIN

buffer_manager::buffer_manager()
    : impl_(std::make_unique<implementation>())
{
}

buffer_manager::~buffer_manager()
{
    impl_->deinitialize();
}

bool buffer_manager::initialize(device& dev) const
{
    return impl_->initialize(dev);
}

void buffer_manager::deinitialize() const noexcept
{
    impl_->deinitialize();
}

void buffer_manager::set_releaser(deferred_releaser* r) const noexcept
{
    impl_->set_releaser(r);
}

void buffer_manager::set_upload_arena(upload_arena* a) const noexcept
{
    impl_->set_upload_arena(a);
}

buffer_handle buffer_manager::create(const buffer_create_info& info) const
{
    return impl_->create(info);
}

std::vector<buffer_handle> buffer_manager::create_batch(
    const std::span<const buffer_create_info> infos) const
{
    return impl_->create_batch(infos);
}

void buffer_manager::destroy(const buffer_handle h) const
{
    impl_->destroy(h);
}

ID3D12Resource* buffer_manager::resource(const buffer_handle h) const
{
    return impl_->resource(h);
}

std::uint64_t buffer_manager::gpu_address(const buffer_handle h) const
{
    return impl_->gpu_address(h);
}

std::uint64_t buffer_manager::size(const buffer_handle h) const
{
    return impl_->size(h);
}

std::uint64_t buffer_manager::stride(const buffer_handle h) const
{
    return impl_->stride(h);
}

void* buffer_manager::mapped_ptr(const buffer_handle h) const
{
    return impl_->mapped_ptr(h);
}

#pragma endregion

#pragma region BUFFER_MANAGER_IMPLEMENTATION

buffer_manager::implementation::~implementation()
{
    deinitialize();
}

bool buffer_manager::implementation::initialize(device& dev)
{
    device_ = &dev;

    slots_.reserve(64);

    if (slots_.empty())
    {
        //~ slot zero invalid
        slots_.push_back(slot{});
    }

    if (!device_->allocator())
    {
        LOG_ERROR("[buffer] initialize failed allocator is null");
        return false;
    }

    return true;
}

void buffer_manager::implementation::deinitialize() noexcept
{
    for (auto& target : slots_)
    {
        release_slot_inline(target);
    }

    slots_.clear();

    device_   = nullptr;
    releaser_ = nullptr;
    arena_    = nullptr;

    next_generation_ = 1;
}

void buffer_manager::implementation::set_releaser(deferred_releaser* releaser) noexcept
{
    releaser_ = releaser;
}

void buffer_manager::implementation::set_upload_arena(upload_arena* arena) noexcept
{
    arena_ = arena;
}

std::uint32_t buffer_manager::implementation::acquire_slot()
{
    for (std::uint32_t i = 1; i < slots_.size(); ++i)
    {
        const slot& target = slots_[i];

        if (target.generation == 0 &&
            target.resource   == nullptr &&
            target.allocation == nullptr &&
            target.mapped     == nullptr)
        {
            return i;
        }
    }

    slots_.push_back(slot{});
    return static_cast<std::uint32_t>(slots_.size() - 1);
}

buffer_manager::implementation::allocation_result
buffer_manager::implementation::allocate_only(const buffer_create_info& info)
{
    if (!device_)
    {
        LOG_ERROR("[buffer] allocate failed device is null");
        return {};
    }

    if (!device_->allocator())
    {
        LOG_ERROR("[buffer] allocate failed allocator is null");
        return {};
    }

    if (info.size_bytes == 0)
    {
        LOG_ERROR("[buffer] allocate failed size is zero");
        return {};
    }

    const std::uint32_t index = acquire_slot();

    slot& target = slots_[index];

    target.generation = ~0u;
    target.size       = info.size_bytes;
    target.stride     = info.stride;
    target.kind       = info.kind;

    const bool is_constant = info.kind == buffer_kind::constant;

    const bool is_uav =
        info.kind == buffer_kind::skinning_output ||
        info.kind == buffer_kind::default_uav;

    D3D12MA::ALLOCATION_DESC alloc_desc{};
    alloc_desc.HeapType = is_constant
        ? D3D12_HEAP_TYPE_UPLOAD
        : D3D12_HEAP_TYPE_DEFAULT;

    const std::uint64_t allocation_size =
        helpers::adjust_to_256(info.size_bytes, is_constant);

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment        = 0;
    desc.Width            = allocation_size;
    desc.Height           = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc       = { 1, 0 };
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags            = is_uav
        ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        : D3D12_RESOURCE_FLAG_NONE;

    const D3D12_RESOURCE_STATES initial_state = is_constant
        ? D3D12_RESOURCE_STATE_GENERIC_READ
        : D3D12_RESOURCE_STATE_COMMON;

    const HRESULT hr = device_->allocator()->CreateResource(
        &alloc_desc,
        &desc,
        initial_state,
        nullptr,
        &target.allocation,
        IID_PPV_ARGS(&target.resource)
    );

    if (FAILED(hr))
    {
        LOG_ERROR(
            "[buffer] CreateResource failed name={} hr=0x{:08X}",
            info.debug_name ? info.debug_name : "<null>",
            static_cast<std::uint32_t>(hr)
        );

        target = slot{};
        return {};
    }

    wchar_t wide_name[64]{};
    swprintf_s(wide_name, L"%hs", info.debug_name ? info.debug_name : "buffer");
    target.resource->SetName(wide_name);

    if (is_constant)
    {
        constexpr D3D12_RANGE no_read{ 0, 0 };

        if (const HRESULT map_hr = target.resource->Map(0, &no_read, &target.mapped); FAILED(map_hr))
        {
            LOG_ERROR(
                "[buffer] constant buffer Map failed name={} hr=0x{:08X}",
                info.debug_name ? info.debug_name : "<null>",
                static_cast<std::uint32_t>(map_hr)
            );

            if (target.allocation)
                target.allocation->Release();

            target = slot{};
            return {};
        }

        if (info.initial_data)
        {
            std::memcpy(target.mapped, info.initial_data, info.size_bytes);
        }
    }

    return { index, true };
}

buffer_handle buffer_manager::implementation::create(const buffer_create_info& info)
{
    const allocation_result alloc = allocate_only(info);
    if (!alloc.ok)
        return buffer_handle::invalid();

    slot& target = slots_[alloc.index];

    if (info.kind != buffer_kind::constant && info.initial_data)
    {
        if (!upload_static(target, info.initial_data, info.size_bytes))
        {
            release_slot_inline(target);
            return buffer_handle::invalid();
        }
    }

    const std::uint32_t generation = next_generation_++;

    if (next_generation_ == 0)
    {
        next_generation_ = 1;
    }

    target.generation = generation;

    return buffer_handle{ alloc.index, generation };
}

std::vector<buffer_handle> buffer_manager::implementation::create_batch(
    std::span<const buffer_create_info> infos)
{
    std::vector<buffer_handle> output;
    output.reserve(infos.size());

    std::vector<std::uint32_t> allocation_indices;
    allocation_indices.reserve(infos.size());

    for (const auto& info : infos)
    {
        const allocation_result allocation = allocate_only(info);

        if (!allocation.ok)
        {
            for (const std::uint32_t index : allocation_indices)
            {
                release_slot_inline(slots_[index]);
            }

            return {};
        }

        allocation_indices.push_back(allocation.index);
    }

    std::vector<upload_record> uploads;
    uploads.reserve(infos.size());

    for (std::size_t i = 0; i < infos.size(); ++i)
    {
        const buffer_create_info& info = infos[i];

        if (info.kind == buffer_kind::constant || !info.initial_data)
            continue;

        slot& target = slots_[allocation_indices[i]];

        upload_record record{};
        record.dst  = &target;
        record.data = info.initial_data;
        record.size = info.size_bytes;

        uploads.push_back(record);
    }

    if (!uploads.empty())
    {
        if (!upload_batch(uploads))
        {
            LOG_ERROR("[buffer] batch upload failed");

            for (const std::uint32_t index : allocation_indices)
            {
                release_slot_inline(slots_[index]);
            }

            return {};
        }
    }

    for (const std::uint32_t index : allocation_indices)
    {
        slot& target = slots_[index];

        const std::uint32_t generation = next_generation_++;

        if (next_generation_ == 0)
        {
            next_generation_ = 1;
        }

        target.generation = generation;

        output.push_back(buffer_handle{ index, generation });
    }

    LOG_INFO("[buffer] batch created {} buffers in one flush", output.size());

    return output;
}

bool buffer_manager::implementation::upload_batch(
    std::span<const upload_record> records) const
{
    if (records.empty())
        return true;

    if (!arena_)
    {
        LOG_ERROR("[buffer] upload_batch called before upload arena wired");
        return false;
    }

    if (!arena_->begin_batch())
    {
        LOG_ERROR("[buffer] arena begin_batch failed");
        return false;
    }

    for (const auto& record : records)
    {
        if (!record.dst || !record.dst->resource || !record.data || record.size == 0)
        {
            LOG_ERROR("[buffer] upload record invalid");
            arena_->cancel_batch();
            return false;
        }

        const D3D12_RESOURCE_STATES state = end_state_for(record.dst->kind);

        if (!arena_->add_buffer_copy(
                record.dst->resource,
                record.data,
                record.size,
                state))
        {
            LOG_ERROR("[buffer] arena add_buffer_copy failed");
            arena_->cancel_batch();
            return false;
        }
    }

    return arena_->submit_and_wait();
}

bool buffer_manager::implementation::upload_static(
    const slot& target,
    const void* data,
    const std::uint64_t upload_size) const
{
    if (!arena_)
    {
        LOG_ERROR("[buffer] upload_static called before upload arena wired");
        return false;
    }

    if (!target.resource || !data || upload_size == 0)
    {
        LOG_ERROR("[buffer] upload_static invalid arguments");
        return false;
    }

    if (!arena_->begin_batch())
    {
        LOG_ERROR("[buffer] arena begin_batch failed");
        return false;
    }

    const D3D12_RESOURCE_STATES state = end_state_for(target.kind);

    if (!arena_->add_buffer_copy(target.resource, data, upload_size, state))
    {
        LOG_ERROR("[buffer] arena add_buffer_copy failed");
        arena_->cancel_batch();
        return false;
    }

    return arena_->submit_and_wait();
}

void buffer_manager::implementation::destroy(const buffer_handle handle)
{
    if (!handle.valid() || handle.index >= slots_.size())
        return;

    slot& target = slots_[handle.index];

    if (target.generation != handle.generation)
        return;

    if (target.mapped && target.resource)
    {
        target.resource->Unmap(0, nullptr);
        target.mapped = nullptr;
    }

    if (releaser_)
    {
        releaser_->enqueue_allocation(target.allocation);
        target.allocation = nullptr;
    }
    else if (target.allocation)
    {
        target.allocation->Release();
        target.allocation = nullptr;
    }

    target = slot{};
}

ID3D12Resource* buffer_manager::implementation::resource(
    const buffer_handle handle) const
{
    if (!handle.valid() || handle.index >= slots_.size())
        return nullptr;

    const slot& target = slots_[handle.index];

    return target.generation == handle.generation
        ? target.resource
        : nullptr;
}

std::uint64_t buffer_manager::implementation::gpu_address(
    const buffer_handle handle) const
{
    ID3D12Resource* target = resource(handle);

    return target
        ? target->GetGPUVirtualAddress()
        : 0;
}

std::uint64_t buffer_manager::implementation::size(const buffer_handle handle) const
{
    if (!handle.valid() || handle.index >= slots_.size())
        return 0;

    const slot& target = slots_[handle.index];

    return target.generation == handle.generation
        ? target.size
        : 0;
}

std::uint64_t buffer_manager::implementation::stride(const buffer_handle handle) const
{
    if (!handle.valid() || handle.index >= slots_.size())
        return 0;

    const slot& target = slots_[handle.index];

    return target.generation == handle.generation
        ? target.stride
        : 0;
}

void* buffer_manager::implementation::mapped_ptr(const buffer_handle handle) const
{
    if (!handle.valid() || handle.index >= slots_.size())
        return nullptr;

    const slot& target = slots_[handle.index];

    return target.generation == handle.generation
        ? target.mapped
        : nullptr;
}

void buffer_manager::implementation::release_slot_inline(slot& target) noexcept
{
    if (target.mapped && target.resource)
    {
        target.resource->Unmap(0, nullptr);
        target.mapped = nullptr;
    }

    if (target.allocation)
    {
        target.allocation->Release();
        target.allocation = nullptr;
    }

    target = slot{};
}

#pragma endregion
