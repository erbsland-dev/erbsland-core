// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::resource {

/// A machine-readable resource lookup failure category.
enum class ResourceErrorCategory : uint8_t {
    NotFound,   ///< No resource exists for the requested identifier and path.
    InvalidData ///< The stored resource representation cannot be decoded.
};

}
