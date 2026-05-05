// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::debug {

/// Flags for controlling debug view details.
/// @tested{StringDebugTest}
enum class DebugViewDetail : uint16_t {
    None = 0U, ///< No optional details.
    // categories:
    CoreDetails = 1U << 0U, ///< Category with the core information, also the default.
    // details:
    ContentInTitle = 1U << 1U, ///< Include a short safe content preview in the tree title.
    States = 1U << 2U,         ///< Core states, like emptiness.
    Size = 1U << 3U,           ///< The size of a container in elements (not memory).
    Range = 1U << 4U,          ///< The range an object represents or contains.
    BackingStore = 1U << 5U,   ///< Details of the backing storage used by the object.
    DataValidity = 1U << 6U,   ///< If the data in the object is valid.
    UnderlyingType = 1U << 7U, ///< Details of the underlying type if an object is a wrapper.
    StorageId = 1U << 8U,      ///< A unique identifier for the storage used by the object.
    HashInfo = 1U << 9U,       ///< Information about the hash of the object.
    Default = CoreDetails,     ///< The default set of details.
    All = ((1U << 10U) - 1U),  ///< All flags set.
};

/// A set of debug view detail flags.
using DebugViewDetails = util::EnumFlags<DebugViewDetail>;

/// Combine two debug view detail flags.
[[nodiscard]] constexpr auto operator|(const DebugViewDetail left, const DebugViewDetail right) noexcept
    -> DebugViewDetails {
    return DebugViewDetails{left} | right;
}

}
