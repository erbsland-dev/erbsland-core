// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The returned status of a text node walk function.
enum class TextWalkStatus : uint8_t {
    Continue, ///< Continue with the next node.
    Stop,     ///< Stop the walk successfully.
    Failure,  ///< Stop the walk because of a failure.
};

}
