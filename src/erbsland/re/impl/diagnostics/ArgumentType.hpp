// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::re::impl {

/// The native type of the argument.
enum class ArgumentType : uint8_t {
    Text,    ///< A text.
    Integer, ///< An integer.
    Boolean, ///< A boolean.
};

}
