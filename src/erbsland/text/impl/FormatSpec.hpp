// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatPresentation.hpp"

#include "../Char.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"
#include "../IntegerSignMode.hpp"
#include "../LetterCase.hpp"

#include "../../bgeo/AlignmentFlags.hpp"
#include "../../unit/CpLength.hpp"

#include <optional>

namespace erbsland::text::impl {

/// Parsed format specification for one replacement field.
/// @tested{U8FormatTest}
struct FormatSpec final {
    bgeo::AlignmentFlag alignment{bgeo::AlignmentFlag::None};     ///< The requested field alignment.
    Char fill{U' '};                                              ///< The field fill character.
    IntegerSignMode signMode{IntegerSignMode::NegativeOnly};      ///< The integer sign mode.
    bool alternateForm{false};                                    ///< Whether alternate numeric form was requested.
    bool zeroFill{false};                                         ///< Whether leading zero-fill was requested.
    std::optional<unit::CpLength> width{};                        ///< The optional field width.
    std::optional<unit::CpLength> precision{};                    ///< The optional precision.
    FormatPresentation presentation{FormatPresentation::Default}; ///< The presentation type.
    LetterCase letterCase{LetterCase::Lowercase};                 ///< The letter case for lettered presentations.
    EscapeFormat escapeFormat{EscapeFormat::None};                ///< The escape format for `/` presentations.
    EscapeAmount escapeAmount{EscapeAmount::Balanced};            ///< The escape amount for `/` presentations.
};

}
