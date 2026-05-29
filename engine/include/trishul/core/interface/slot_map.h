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
#ifndef CURSEOFTHESEA_SLOT_MAP_H
#define CURSEOFTHESEA_SLOT_MAP_H


#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace trishul
{
    template<class Tag>
    struct handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]]        constexpr bool   valid  () const noexcept { return generation != 0u; }
        [[nodiscard]] static constexpr handle invalid()       noexcept { return {}; }

        constexpr bool operator==(const handle&) const noexcept = default;
    };

    template<class T, class Tag = T>
    class slot_map
    {
    public:
        using handle_type = handle<Tag>;

        template<class... Args>
        handle_type emplace(Args&&... args)
        {
            if (!free_.empty())
            {
                const std::uint32_t i = free_.back();
                free_.pop_back();
                slot& s = slots_[i];
                s.value.emplace(std::forward<Args>(args)...);
                ++size_;
                return { i, s.generation };
            }

            const auto i = static_cast<std::uint32_t>(slots_.size());
            slot s{};
            s.value.emplace(std::forward<Args>(args)...);
            s.generation = 1u; //~ first life of a fresh slot
            slots_.push_back(std::move(s));
            ++size_;
            return { i, slots_[i].generation };
        }

        handle_type insert(T value) { return emplace(std::move(value)); }

        //~ resolve a handle to its element or null when the handle is
        //  invalid out of range, freed or points at a recycled slot
        [[nodiscard]] T* get(const handle_type h) noexcept
        {
            slot* s = live_slot(h);
            return s ? &*s->value : nullptr;
        }
        [[nodiscard]] const T* get(const handle_type h) const noexcept
        {
            const slot* s = live_slot(h);
            return s ? &*s->value : nullptr;
        }

        [[nodiscard]] bool contains(const handle_type h) const noexcept
        {
            return live_slot(h) != nullptr;
        }

        bool erase(const handle_type h) noexcept
        {
            slot* s = live_slot(h);
            if (!s) return false;
            s->value.reset();
            ++s->generation;
            if (s->generation == 0u) s->generation = 1u;
            free_.push_back(h.index);
            --size_;
            return true;
        }

        template<class Fn>
        void for_each(Fn&& fn)
        {
            for (slot& s : slots_) if (s.value) fn(*s.value);
        }
        template<class Fn>
        void for_each(Fn&& fn) const
        {
            for (const slot& s : slots_) if (s.value) fn(*s.value);
        }

        [[nodiscard]] std::uint32_t size()  const noexcept { return size_; }
        [[nodiscard]] bool          empty() const noexcept { return size_ == 0u; }

        void clear() noexcept
        {
            slots_.clear();
            free_.clear();
            size_ = 0u;
        }

    private:
        struct slot
        {
            std::optional<T> value;
            std::uint32_t    generation { 0u };
        };

        [[nodiscard]] slot* live_slot(const handle_type h) noexcept
        {
            if (!h.valid() || h.index >= slots_.size()) return nullptr;
            slot& s = slots_[h.index];
            return (s.value && s.generation == h.generation) ? &s : nullptr;
        }
        [[nodiscard]] const slot* live_slot(const handle_type h) const noexcept
        {
            if (!h.valid() || h.index >= slots_.size()) return nullptr;
            const slot& s = slots_[h.index];
            return (s.value && s.generation == h.generation) ? &s : nullptr;
        }

        std::vector<slot>          slots_;
        std::vector<std::uint32_t> free_;
        std::uint32_t              size_ { 0u };
    };
} // namespace trishul

#endif //CURSEOFTHESEA_SLOT_MAP_H
