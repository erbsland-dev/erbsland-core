// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// The order in which bits are processed within a byte and integer field.
enum class BitOrder : uint8_t {
    MostSignificantFirst,  ///< Process the highest-order bit first.
    LeastSignificantFirst, ///< Process the lowest-order bit first.
};

}
