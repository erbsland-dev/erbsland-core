// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatPresentation.hpp"

#include "../BooleanFormat.hpp"
#include "../Char.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"
#include "../FloatFormat.hpp"
#include "../IntegerBase.hpp"
#include "../IntegerSignMode.hpp"
#include "../LetterCase.hpp"
#include "../TruncateMode.hpp"

#include "../../bgeo/AlignmentFlags.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/CpLength.hpp"

#include <optional>

namespace erbsland::text::impl {

/// Layout shared by named text-like and numeric format specifications.
/// @tested{U8FormatTest}
struct NamedLayoutSpec final {
    bgeo::AlignmentFlag alignment{bgeo::AlignmentFlag::None}; ///< The requested field alignment.
    Char fill{U' '};                                          ///< The field fill character.
    std::optional<unit::CpLength> width{};                    ///< The optional field width.
};

/// Legacy compact format specification.
/// @tested{U8FormatTest}
struct LegacyFormatSpec final {
    bgeo::AlignmentFlag alignment{bgeo::AlignmentFlag::None};     ///< The requested field alignment.
    Char fill{U' '};                                              ///< The field fill character.
    IntegerSignMode signMode{IntegerSignMode::NegativeOnly};      ///< The integer sign mode.
    bool alternateForm{false};                                    ///< Whether alternate numeric form was requested.
    bool zeroFill{false};                                         ///< Whether leading zero-fill was requested.
    std::optional<unit::CpLength> width{};                        ///< The optional field width.
    std::optional<unit::CpLength> precision{};                    ///< The optional precision.
    FormatPresentation presentation{FormatPresentation::Default}; ///< The presentation type.
    LetterCase letterCase{LetterCase::Lowercase};                 ///< The case for lettered presentations.
    EscapeFormat escapeFormat{EscapeFormat::None};                ///< The escape format for `/` presentations.
    EscapeAmount escapeAmount{EscapeAmount::Balanced};            ///< The escape amount for `/` presentations.
};

/// Named text format specification.
/// @tested{U8FormatTest}
struct NamedTextFormatSpec final {
    NamedLayoutSpec layout;                            ///< The output layout.
    std::optional<unit::CpLength> maximum{};           ///< The optional source code-point maximum.
    std::optional<EscapeFormat> escapeFormat{};        ///< The optional escape format.
    EscapeAmount escapeAmount{EscapeAmount::Balanced}; ///< The escape amount.
    bool hasEscapeAmount{false};                       ///< Whether the escape amount was explicitly selected.
};

/// Named number format specification.
/// @tested{U8FormatTest}
struct NamedNumberFormatSpec final {
    NamedLayoutSpec layout;                                  ///< The output layout.
    std::optional<IntegerBase> base{};                       ///< The optional integer base.
    std::optional<FloatFormat::Style> notation{};            ///< The optional floating-point notation.
    LetterCase letterCase{LetterCase::Lowercase};            ///< The case for generated ASCII letters.
    IntegerSignMode signMode{IntegerSignMode::NegativeOnly}; ///< The sign mode.
    std::optional<unit::CpLength> precision{};               ///< The optional precision.
    bool alternateForm{false};                               ///< Whether alternate integer form is enabled.
    bool zeroFill{false};                                    ///< Whether numeric zero filling is enabled.
};

/// Named boolean format specification.
/// @tested{U8FormatTest}
struct NamedBooleanFormatSpec final {
    NamedLayoutSpec layout; ///< The output layout.
    BooleanFormat format;   ///< The boolean word format.
};

/// Named byte-block format specification.
/// @tested{U8FormatTest}
struct NamedBytesFormatSpec final {
    bool separator{false};                                  ///< Whether bytes are separated.
    unit::ByteLength maximum{unit::ByteLength::infinite()}; ///< The maximum output item count.
    TruncateMode truncateMode{TruncateMode::End};           ///< The truncation mode.
};

}
