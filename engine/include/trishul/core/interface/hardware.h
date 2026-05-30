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
#ifndef CURSEOFTHESEA_HARDWARE_H
#define CURSEOFTHESEA_HARDWARE_H

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>

namespace trishul::render::hardware
{
    namespace detail
    {
        //~ an unique address per type a no rtti stand in for type id
        template<typename T>
        struct config_tag { static constexpr char value = 0; };
    }

    template<typename T>
    [[nodiscard]] constexpr const void* config_type_id() noexcept
    {
        return &detail::config_tag<std::remove_cvref_t<T>>::value;
    }

    //~ owns a copy of a trivially copyable pod and only handing it back
    //~ as an exact type that was stored wrong type or empty returns null
    class hardware_config
    {
    public:
        hardware_config() = default;

        template<typename T>
            requires (std::is_trivially_copyable_v<std::remove_cvref_t<T>>
                   && !std::is_same_v<std::remove_cvref_t<T>, hardware_config>)
        explicit hardware_config(const T& value) noexcept
            : type_{ config_type_id<T>() }, size_{ sizeof(T) }
        {
            static_assert(sizeof(T)  <= buffer_size,
                "hardware config pod too large bump buffer_size");
            static_assert(alignof(T) <= alignof(std::max_align_t),
                "over aligned hardware config pod not supported");
            std::memcpy(buffer_, &value, sizeof(T));
        }

        template<typename T>
        [[nodiscard]] const T* as() const noexcept
        {
            if (type_ == nullptr || type_ != config_type_id<T>()) return nullptr;
            return std::launder(reinterpret_cast<const T*>(buffer_));
        }

        [[nodiscard]] bool empty() const noexcept { return type_ == nullptr; }

    private:
        static constexpr std::size_t buffer_size = 64u;

        alignas(std::max_align_t) std::byte buffer_[buffer_size]{};
        const void* type_{ nullptr };
        std::size_t size_{ 0u };
    };

    //~ every gpu resource owner
    //~ the renderer can bring them up in dependency order and rebuild on demand
    class __declspec(novtable) interfaces
    {
    public:
        virtual ~interfaces() = default;

        [[nodiscard]]
        virtual bool initialize  ()                = 0;
        virtual void deinitialize()       noexcept = 0;

        //~ child raises this when its gpu objects went stale
                      virtual bool need_rebuild() const noexcept = 0;
        [[nodiscard]] virtual const char* name () const noexcept { return "hardware"; }

        //~ hand the child its pod config before initialize type checked and owned
        template<typename T>
        void set_config(const T& config) noexcept { config_ = hardware_config{ config }; }

    protected:
        //~ read config null when unset or the wrong type was stored
        template<typename T>
        [[nodiscard]] const T* config_as() const noexcept { return config_.as<T>(); }

        hardware_config config_{};
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_HARDWARE_H
