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
    void flushStaticText();
    void requireFieldLimit() const;
    [[nodiscard]] auto readIndex() -> std::optional<unit::ArgumentIndex>;
    [[nodiscard]] auto currentChar() const noexcept -> Char;
    auto consumeChar() noexcept -> Char;
    auto consumeSpecificationChar() -> Char;
    [[nodiscard]] auto readLimitedDecimal(const StringLiteral &tooLargeMessage) -> unit::CpLength;
    [[nodiscard]] auto parseSpecification() -> FormatSpec;
    void parseAlignment(FormatSpec &spec);
    void parseSign(FormatSpec &spec);
    void parseWidth(FormatSpec &spec);
    void parsePrecision(FormatSpec &spec);
    void parsePresentation(FormatSpec &spec);
    void parseEscapedPresentation(FormatSpec &spec);
    [[nodiscard]] auto resolveArgumentIndex(std::optional<unit::ArgumentIndex> explicitIndex) -> unit::ArgumentIndex;
    void markArgumentIndex(unit::ArgumentIndex argumentIndex);
    void parseField();
    [[nodiscard]] auto finish() -> FormatDataPtr;

private:
    StringCharReader _reader;
    AnyStringBuilder _staticText;
    FormatDataPtr _data;
    IndexMode _indexMode{IndexMode::None};
    unit::ArgumentIndex _nextAutomaticIndex{unit::ArgumentIndex::zero()};
};

}
