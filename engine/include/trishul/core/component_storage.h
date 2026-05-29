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
#ifndef CURSEOFTHESEA_COMPONENT_STORAGE_H
#define CURSEOFTHESEA_COMPONENT_STORAGE_H

#include "trishul/core/engine_assert.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace trishul::component
{
    // specialize per tag with two members
    template<typename Tag>
    struct traits;

    template<typename Tag>
    concept tag = requires
    {
        typename traits<Tag>::type;
        typename traits<Tag>::owner;
    };

    template<tag Tag>
    using component_t = typename traits<Tag>::type;

    template<tag Tag>
    using owner_t = typename traits<Tag>::owner;

    //~ tag belongs to host only
    template<typename Tag, typename Host>
    concept component_of = tag<Tag> && std::same_as<owner_t<Tag>, Host>;

    template<tag... Tags>
    class storage
    {
        static_assert(sizeof...(Tags) > 0,
            "component_storage needs at least one component tag");

        static constexpr std::size_t npos = static_cast<std::size_t>(-1);

        template<typename Tag>
        static constexpr std::size_t index_of() noexcept
        {
            std::size_t result = npos;
            std::size_t i      = 0;
            (void)((std::is_same_v<Tag, Tags>
                ? (result = i, true)
                : (++i, false)) || ...);
            return result;
        }

        template<typename Tag>
        static constexpr std::size_t count_of() noexcept
        {
            return (static_cast<std::size_t>(std::is_same_v<Tag, Tags>) + ...);
        }

        static_assert((... && (count_of<Tags>() == 1)),
            "duplicate component tag in storage declaration");

        using container = std::tuple<std::unique_ptr<component_t<Tags>>...>;

    public:
         storage() = default;
        ~storage() = default;

        storage(const storage&)            = delete;
        storage(storage&&)                 = delete;
        storage& operator=(const storage&) = delete;
        storage& operator=(storage&&)      = delete;

        //~ is the tag part of this set compile time
        template<typename Tag>
        [[nodiscard]] static constexpr bool contains() noexcept
        {
            return index_of<Tag>() != npos;
        }

        static constexpr std::size_t capacity() noexcept { return sizeof...(Tags); }

        //~ is the component live runtime
        template<typename Tag>
        [[nodiscard]] bool alive() const noexcept
        {
            static_assert(index_of<Tag>() != npos, "tag not in this storage");
            return static_cast<bool>(std::get<index_of<Tag>()>(slots_));
        }

        //~ build in place with runtime args
        template<typename Tag, typename... Args>
        component_t<Tag>& emplace(Args&&... args)
        {
            constexpr std::size_t idx = index_of<Tag>();
            static_assert(idx != npos, "tag not in this storage");

            auto& slot = std::get<idx>(slots_);
            slot = std::make_unique<component_t<Tag>>(std::forward<Args>(args)...);
            return *slot;
        }

        template<typename Tag>
        void erase() noexcept
        {
            static_assert(index_of<Tag>() != npos, "tag not in this storage");
            std::get<index_of<Tag>()>(slots_).reset();
        }

        void clear() noexcept
        {
            std::apply([](auto&... slot)
            {
                (slot.reset(), ...);
            }, slots_);
        }

        //~ unchecked nullable access
        template<typename Tag>
        [[nodiscard]] component_t<Tag>* try_get() noexcept
        {
            static_assert(index_of<Tag>() != npos, "tag not in this storage");
            return std::get<index_of<Tag>()>(slots_).get();
        }

        template<typename Tag>
        [[nodiscard]] const component_t<Tag>* try_get() const noexcept
        {
            static_assert(index_of<Tag>() != npos, "tag not in this storage");
            return std::get<index_of<Tag>()>(slots_).get();
        }

        //~ checked reference access
        template<typename Tag>
        [[nodiscard]] component_t<Tag>& get() noexcept
        {
            auto* ptr = try_get<Tag>();
            ENGINE_VERIFY_MSG(ptr != nullptr, "component used before emplace");
            return *ptr;
        }

        template<typename Tag>
        [[nodiscard]] const component_t<Tag>& get() const noexcept
        {
            const auto* ptr = try_get<Tag>();
            ENGINE_VERIFY_MSG(ptr != nullptr, "component used before emplace");
            return *ptr;
        }

        //~ apply fn to every live component
        template<typename F>
        void for_each(F&& fn)
        {
            std::apply([&](auto&... slot)
            {
                ((slot ? (void)fn(*slot) : void()), ...);
            }, slots_);
        }

        template<typename F>
        void for_each(F&& fn) const
        {
            std::apply([&](const auto&... slot)
            {
                ((slot ? (void)fn(*slot) : void()), ...);
            }, slots_);
        }

    private:
        container slots_{};
    };

    //~ specialize a tag from global or enclosing scope
    #define ENGINE_DECLARE_COMPONENT(TAG, CONCRETE, OWNER)\
        template<> struct ::trishul::component::traits<TAG>\
        {\
            using type  = CONCRETE;\
            using owner = OWNER;\
        }
} // namespace trishul::component

#endif //CURSEOFTHESEA_COMPONENT_STORAGE_H
