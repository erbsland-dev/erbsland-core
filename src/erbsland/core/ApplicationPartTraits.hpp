// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPart.hpp"
#include "ApplicationPartIdentifier_fwd.hpp"

#include <concepts>
#include <memory>
#include <type_traits>

namespace erbsland::core {

/// A public interface that identifies its application part.
template <typename T>
concept ApplicationPartInterface = requires {
    { T::partIdentifier() } -> std::convertible_to<ApplicationPartIdentifierPtr>;
};

/// A concrete class that can be registered as an application part.
template <typename T>
concept ApplicationPartClass = std::derived_from<T, ApplicationPart> && requires {
    { T::partIdentifier() } -> std::convertible_to<ApplicationPartIdentifierPtr>;
    { T::dependencies() } -> std::same_as<ApplicationPartIdentifierList>;
    { T::create() } -> std::same_as<std::shared_ptr<T>>;
};

}
