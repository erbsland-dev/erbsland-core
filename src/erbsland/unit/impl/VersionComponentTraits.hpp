// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../VersionComponent.hpp"
#include "../VersionPart.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::unit::impl {

/// Traits for converting a version component to and from text.
template <typename T>
struct VersionComponentTraits {
    static constexpr auto cIsVersionComponent = false; ///< Whether the type is a version component.
};

/// Traits identifying an independent version component.
template <VersionPart tPart>
struct VersionComponentTraits<VersionComponent<tPart>> {
    static constexpr auto cIsVersionComponent = true; ///< Identifies a version component.
    static constexpr auto cPart = tPart;              ///< The represented field.
};

template <typename T>
concept VersionComponentArgument = VersionComponentTraits<std::remove_cvref_t<T>>::cIsVersionComponent;

template <VersionPart tPart, typename... tArguments>
concept HasSingleVersionPart =
    (((VersionComponentTraits<std::remove_cvref_t<tArguments>>::cPart == tPart) ? 1 : 0) + ...) <= 1;

template <typename... tArguments>
concept UniqueVersionComponentArguments = HasSingleVersionPart<VersionPart::Major, tArguments...> &&
    HasSingleVersionPart<VersionPart::Minor, tArguments...> &&
    HasSingleVersionPart<VersionPart::Revision, tArguments...> &&
    HasSingleVersionPart<VersionPart::Build, tArguments...>;

}
