// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_DEPENDENCY_SCHEDULER_H
#define CURSEOFTHESEA_DEPENDENCY_SCHEDULER_H

#include <unordered_set>
#include <vector>
#include <typeindex>
#include <memory>
#include <queue>
#include <stdexcept>
#include <iterator>
#include <type_traits>
#include <unordered_map>

namespace cots::utils
{
    template<typename T>
    class dependency_scheduler
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

         dependency_scheduler() = default;
        ~dependency_scheduler() = default;

        dependency_scheduler           (const dependency_scheduler&) = delete;
        dependency_scheduler& operator=(const dependency_scheduler&) = delete;

        template<typename U>
        void register_type(U& instance)
        {
            static_assert(std::is_base_of_v<T, U>, "U must be derived from T");
            const std::type_index id{ typeid(U) };

            //~ already registered skipping
            if (nodes_.contains(id))
            {
                return;
            }

            nodes_[id]  = node{ &instance, {} };
            dirty_      = true;
        }

        template<typename Depender, typename...DependsUpon>
        void add_dependency()
        {
            static_assert(std::is_base_of_v<T, Depender>, "Depender must be derived from T");
            static_assert((std::is_base_of_v<T, DependsUpon> && ...), "DependsUpon must be derived from T");

            node& n = require_node(typeid(Depender));
            (add_single_dep(n, typeid(Depender), typeid(DependsUpon)),...);
            dirty_ = true;
        }

        template<typename...DependsUpon>
        void add_dependency(T& depender, DependsUpon&&...deps)
        {
            node& n = require_node(typeid(T));
            (add_single_dep(n, typeid(depender), typeid(deps)),...);
            dirty_ = true;
        }

        size_type size () const noexcept { return nodes_.size(); }
        bool      empty() const noexcept { return nodes_.empty(); }

        iterator begin() noexcept { ensure_sorted(); return sorted_.get(); }
        iterator end  () noexcept { ensure_sorted(); return sorted_.get() + sorted_size_; }

        reverse_iterator rbegin() const { return reverse_iterator{ end()   }; }
        reverse_iterator rend()   const { return reverse_iterator{ begin() }; }

    private:
        node& require_node(const std::type_index& id) const
        {
            const auto it = nodes_.find(id);
            if (it == nodes_.end())
            {
                throw std::runtime_error("No such type registered");
            }
            return it->second;
        }

        void add_single_dep(node& n, const std::type_index self, const std::type_index dep)
        {
            if (not nodes_.contains(dep)) throw std::runtime_error("No such type registered");
            if (dep == self)              throw std::runtime_error("Self-dependency not allowed");
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

            if (out != nodes_.size())
                throw std::runtime_error("Cycle detected in dependency graph");

            sorted_      = std::move(buf);
            sorted_size_ = out;
            dirty_       = false;
        }
    };
}

#endif //CURSEOFTHESEA_DEPENDENCY_SCHEDULER_H
