// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BoxAttributes.hpp"

#include <cstdint>

namespace tool::cterm {

/// One selected terminal frame scalar with source-derived stroke attributes.
/// @tested{BoxFrameToolTest}
struct BoxCharacter {
    char32_t codePoint{};     ///< Selected Unicode scalar.
    BoxAttributes attributes; ///< Interpreted UCD character name.
};

}
