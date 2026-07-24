// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::conf {

/// The result of an access check.
enum class AccessCheckResult : uint8_t {
    /// If the access is granted.
    Granted,
    /// If the access is denied.
    Denied,
};

}
