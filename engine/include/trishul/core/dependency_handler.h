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
#ifndef CURSEOFTHESEA_DEPENDENCY_HANDLER_H
#define CURSEOFTHESEA_DEPENDENCY_HANDLER_H

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <queue>
#include <iterator>
#include <type_traits>
#include <cstddef>

#include "trishul/core/engine_assert.h"

namespace trishul
{
    template<typename T>
    class dependency_handler
    {
        struct node
        {
            T* instance{ nullptr };
            std::unordered_set<std::type_index> dependencies;
        };
        std::unordered_map<std::type_index, node> nodes_;

        mutable std::unique_ptr<T*[]> sorted_;
        mutable std::size_t           sorted_size_{ 0 };
        mutable bool                  dirty_      { true };

    public:
        using value_type             = T*;
        using size_type              = std::size_t;
        using iterator               = T* const*;
        using const_iterator         = T* const*;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

         dependency_handler() = default;
        ~dependency_handler() = default;

        dependency_handler           (const dependency_handler&) = delete;
        dependency_handler& operator=(const dependency_handler&) = delete;

        template<typename U>
        void register_type(U* instance)
        {
            static_assert(std::is_base_of_v<T, U>, "U must be derived from T");
            ENGINE_VERIFY_MSG(instance != nullptr, "register_type null pointer");

            const std::type_index id{ typeid(U) };
            if (nodes_.contains(id)) return;          // already registered skip

            nodes_[id] = node{ instance, {} };
            dirty_     = true;
        }

        template<typename Depender, typename... DependsUpon>
        void add_dependency()
        {
            static_assert(std::is_base_of_v<T, Depender>, "Depender must be derived from T");
            static_assert((std::is_base_of_v<T, DependsUpon> && ...),
                          "Each DependsUpon must be derived from T");

            node& n = require_node(typeid(Depender));
            (add_single_dep(n, typeid(Depender), typeid(DependsUpon)), ...);
            dirty_ = true;
        }

        template<typename U, typename... Us>
        void add_dependency(U* depender, Us*... deps)
        {
            static_assert(std::is_base_of_v<T, U>);
            static_assert((std::is_base_of_v<T, Us> && ...));

            (void)depender;
            ((void)deps, ...);

            node& n = require_node(typeid(U));
            (add_single_dep(n, typeid(U), typeid(Us)), ...);
            dirty_ = true;
        }

        size_type size () const noexcept { return nodes_.size(); }
        bool      empty() const noexcept { return nodes_.empty(); }

        iterator begin() const { ensure_sorted(); return sorted_.get(); }
        iterator end  () const { ensure_sorted(); return sorted_.get() + sorted_size_; }

        reverse_iterator rbegin() const { return reverse_iterator{ end()   }; }
        reverse_iterator rend()   const { return reverse_iterator{ begin() }; }

    private:
        node& require_node(const std::type_index& id)
        {
            const auto it = nodes_.find(id);
            ENGINE_VERIFY_MSG(it != nodes_.end(), "dependency handler no such type registered");
            return it->second;
        }

        void add_single_dep(node& n, std::type_index self, std::type_index dep)
        {
            ENGINE_VERIFY_MSG(nodes_.contains(dep), "dependency handler no such type registered");
            ENGINE_VERIFY_MSG(dep != self,          "dependency handler self dependency not allowed");
            n.dependencies.insert(dep);
        }

        void ensure_sorted() const
        {
            if (!dirty_) return;

            std::unordered_map<std::type_index, int> in_degree;
            std::unordered_map<std::type_index, std::vector<std::type_index>> rev_edges;
            in_degree.reserve(nodes_.size());

            for (const auto& [id, n] : nodes_)
            {
                in_degree[id] = static_cast<int>(n.dependencies.size());
                for (const auto& d : n.dependencies)
                    rev_edges[d].push_back(id);
            }

            std::queue<std::type_index> ready;
            for (const auto& [id, deg] : in_degree)
                if (deg == 0) ready.push(id);

            auto buf = std::make_unique<T*[]>(nodes_.size());
            std::size_t out = 0;

            while (!ready.empty())
            {
                auto id = ready.front(); ready.pop();
                buf[out++] = nodes_.at(id).instance;
                for (const auto& dep : rev_edges[id])
                    if (--in_degree[dep] == 0) ready.push(dep);
            }

            ENGINE_VERIFY_MSG(out == nodes_.size(), "dependency handler cycle detected");

            sorted_      = std::move(buf);
            sorted_size_ = out;
            dirty_       = false;
        }
    };
} // namespace trishul

#endif //CURSEOFTHESEA_DEPENDENCY_HANDLER_H
