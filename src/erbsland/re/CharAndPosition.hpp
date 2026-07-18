// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharAndPosition_fwd.hpp"
#include "InputPosition.hpp"

#include "../text/Char.hpp"

namespace erbsland::re {

/// A read character and its start position.
/// @tested{InputBaseTest}
struct CharAndPosition {
    text::Char character;   ///< The read character or the end-of-data signal.
    InputPosition position; ///< The start position of the read character.
};

}
