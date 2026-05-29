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
#ifndef CURSEOFTHESEA_COMPONENT_HOST_H
#define CURSEOFTHESEA_COMPONENT_HOST_H

#include "trishul/core/component_storage.h"

#include <concepts>
#include <utility>

namespace trishul::component::detail
{
    //~ every tag must name owner as its host
    template<typename Owner, tag... Tags>
    inline constexpr bool all_owned_by =
        (std::same_as<owner_t<Tags>, Owner> && ...);
} // namespace trishul::component::detail

#define ENGINE_GENERATE_COMPONENTS(THIS_CLASS, ...)\
private:\
    using component_host_storage_t = ::trishul::component::storage<__VA_ARGS__>;\
    component_host_storage_t component_host_storage_{};\
    static_assert(\
        ::trishul::component::detail::all_owned_by<THIS_CLASS, __VA_ARGS__>,\
        "every component tag must name " #THIS_CLASS " as its owner");\
public:\
    template<typename Tag>\
    [[nodiscard]] static constexpr bool hosts_component() noexcept\
    { return component_host_storage_t::template contains<Tag>(); }\
    template<typename Tag>\
    [[nodiscard]] bool has_component() const noexcept\
    { return component_host_storage_.template alive<Tag>(); }\
    template<typename Tag>\
    [[nodiscard]] ::trishul::component::component_t<Tag>& get_component() noexcept\
    { return component_host_storage_.template get<Tag>(); }\
    template<typename Tag>\
    [[nodiscard]] const ::trishul::component::component_t<Tag>& get_component() const noexcept\
    { return component_host_storage_.template get<Tag>(); }\
    template<typename Tag>\
    [[nodiscard]] ::trishul::component::component_t<Tag>* try_get_component() noexcept\
    { return component_host_storage_.template try_get<Tag>(); }\
    template<typename Tag>\
    [[nodiscard]] const ::trishul::component::component_t<Tag>* try_get_component() const noexcept\
    { return component_host_storage_.template try_get<Tag>(); }\
    template<typename F> void for_each_component(F&& fn)\
    { component_host_storage_.for_each(::std::forward<F>(fn)); }\
    template<typename F> void for_each_component(F&& fn) const\
    { component_host_storage_.for_each(::std::forward<F>(fn)); }\
private:\
    template<typename Tag, typename... Args>\
    ::trishul::component::component_t<Tag>& construct_component(Args&&... args)\
    { return component_host_storage_.template emplace<Tag>(::std::forward<Args>(args)...); }\
    template<typename Tag>\
    void destroy_component() noexcept\
    { component_host_storage_.template erase<Tag>(); }\
    void destroy_all_components() noexcept\
    { component_host_storage_.clear(); }

#endif //CURSEOFTHESEA_COMPONENT_HOST_H
