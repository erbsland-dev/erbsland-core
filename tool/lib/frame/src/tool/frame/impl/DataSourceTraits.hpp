// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../DataSource.hpp"

#include <erbsland/String.hpp>

#include <concepts>

namespace tool::frame::impl {

/// A concrete source that the application can register and retrieve by its static identifier.
/// @tparam T The concrete source type with a public static String identifier.
/// @tested{DataSourceTest}
template <typename T>
concept DataSourceType = std::derived_from<T, DataSource> && std::default_initializable<T> && requires {
    { T::sourceIdentifier() } -> std::same_as<el::String>;
};

}
