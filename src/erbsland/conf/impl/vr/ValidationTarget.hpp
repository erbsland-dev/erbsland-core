// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::impl {

/// The target of a constraint validation.
/// *What* is validated by the constraint.
enum class ValidationTarget : uint8_t {
    Value,
    Name,
};

}
