// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerUnitIndex.hpp"
#include "../VersionPart.hpp"
#include "../VersionUnit.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::unit::impl {

template <typename T>
struct VersionComponentTraits {
    static constexpr auto cIsVersionComponent = false; ///< Whether the type is a version component.
};

template <ValidIntegerUnit tIntegerUnit>
    requires(std::derived_from<tIntegerUnit, VersionUnit> && requires { tIntegerUnit::cPart; })
struct VersionComponentTraits<IntegerUnitIndex<tIntegerUnit>> {
    static constexpr auto cIsVersionComponent = true;  ///< Whether the type is a version component.
    static constexpr auto cPart = tIntegerUnit::cPart; ///< The version part represented by the component.
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
