// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatData.hpp"
#include "IndexMode.hpp"

#include "../AnyStringBuilder.hpp"
#include "../Char.hpp"
#include "../StringCharReader.hpp"
#include "../StringLiteral.hpp"
#include "../u8/U8String.hpp"

#include "../../unit/ArgumentUnit.hpp"

#include <cstddef>
#include <optional>

namespace erbsland::text::impl {

/// Parser for compiled UTF-8 format patterns.
/// @tested{U8FormatTest}
class FormatParser final {
    static constexpr auto cMaximumFields = unit::ArgumentCount{128U};
    static constexpr auto cMaximumFieldWidth = unit::CpLength{1024U};
    static constexpr auto cMaximumDecimalDigits = unit::CpLength{4U};
    static constexpr auto cMaximumArgumentIndexDigits = unit::CpLength{3U};

public:
    /// Create a parser for the given format pattern.
    explicit FormatParser(const U8String &pattern);
    /// Create a parser for the given format pattern.
    explicit FormatParser(const U16String &pattern);
    /// Create a parser for the given format pattern.
    explicit FormatParser(const U32String &pattern);

public:
    /// Parse the pattern and return the newly allocated compiled data.
    [[nodiscard]] auto parse() -> FormatDataPtr;

private:
    /// Append accumulated literal text to the parsed format data.
    void flushStaticText();
    /// Throw if the number of fields exceeds its limit.
    void requireFieldLimit() const;
    /// Read an optional explicit argument index.
    [[nodiscard]] auto readIndex() -> std::optional<unit::ArgumentIndex>;
    /// Get the current pattern character.
    [[nodiscard]] auto currentChar() const noexcept -> Char;
    /// Consume and return the current pattern character.
    auto consumeChar() noexcept -> Char;
    /// Consume one character from a format specification.
    auto consumeSpecificationChar() -> Char;
    /// Read a decimal value subject to its supported digit limit.
    [[nodiscard]] auto readLimitedDecimal(const StringLiteral &tooLargeMessage) -> unit::CpLength;
    /// Parse a field format specification.
    [[nodiscard]] auto parseSpecification() -> FormatSpec;
    /// Parse a legacy field format specification.
    [[nodiscard]] auto parseLegacySpecification() -> LegacyFormatSpec;
    /// Parse legacy alignment into `spec`.
    void parseAlignment(LegacyFormatSpec &spec);
    /// Parse legacy sign handling into `spec`.
    void parseSign(LegacyFormatSpec &spec);
    /// Parse legacy width into `spec`.
    void parseWidth(LegacyFormatSpec &spec);
    /// Parse legacy precision into `spec`.
    void parsePrecision(LegacyFormatSpec &spec);
    /// Parse legacy presentation into `spec`.
    void parsePresentation(LegacyFormatSpec &spec);
    /// Parse escaped legacy presentation into `spec`.
    void parseEscapedPresentation(LegacyFormatSpec &spec);
    /// Resolve an optional explicit argument index.
    [[nodiscard]] auto resolveArgumentIndex(std::optional<unit::ArgumentIndex> explicitIndex) -> unit::ArgumentIndex;
    /// Record an argument index as used.
    void markArgumentIndex(unit::ArgumentIndex argumentIndex);
    /// Parse one replacement field.
    void parseField();
    /// Finish parsing and return the compiled format data.
    [[nodiscard]] auto finish() -> FormatDataPtr;

private:
    StringCharReader _reader;
    AnyStringBuilder _staticText;
    FormatDataPtr _data;
    IndexMode _indexMode{IndexMode::None};
    unit::ArgumentIndex _nextAutomaticIndex{unit::ArgumentIndex::zero()};
};

}
