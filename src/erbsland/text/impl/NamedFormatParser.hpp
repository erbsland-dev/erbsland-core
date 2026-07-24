// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatSpec.hpp"
#include "NamedFormatDomain.hpp"
#include "NamedKeyEntry.hpp"
#include "NamedKeyFormat.hpp"

#include "../StringCharReader.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::text::impl {

/// Parser for typed named format specifications.
/// @tested{U8FormatTest U16FormatTest U32FormatTest}
class NamedFormatParser final {
    enum class Option : uint8_t {
        Width,
        Alignment,
        Fill,
        Maximum,
        Escape,
        EscapeAmount,
        Base,
        Notation,
        LetterCase,
        Sign,
        Precision,
        Alternate,
        ZeroFill,
        Style,
        Capitalization,
        Separator,
        Truncate,
    };

public:
    /// Create a parser at the first named option.
    NamedFormatParser(StringCharReader &reader, NamedFormatDomain domain);

public:
    /// Try to consume a typed selector and its trailing colon.
    [[nodiscard]] static auto tryReadDomain(StringCharReader &reader) -> std::optional<NamedFormatDomain>;
    /// Parse the named options and consume the closing field brace.
    [[nodiscard]] auto parse() -> FormatSpec;

private:
    [[nodiscard]] static auto namedKeyFormat() -> const NamedKeyFormat &;
    [[nodiscard]] static auto domainFromName(const String &name) -> std::optional<NamedFormatDomain>;
    [[nodiscard]] static auto isAllowed(Option option, NamedFormatDomain domain) noexcept -> bool;
    [[nodiscard]] auto readIdentifier() const -> String;
    [[nodiscard]] auto readNumber() -> unit::CpLength;
    [[nodiscard]] auto readFill() const -> Char;
    void parseOption(Option option, const NamedKeyEntry &entry);
    void requireKeyWithoutValue() const;
    void parseWidth();
    void parseAlignment();
    void parseFill();
    void parseMaximum();
    void parseEscape();
    void parseEscapeAmount();
    void parseBase();
    void parseNotation();
    void parseLetterCase();
    void parseSign();
    void parsePrecision();
    void parseAlternate();
    void parseZeroFill();
    void parseStyle();
    void parseCapitalization();
    void parseSeparator();
    void parseTruncate();
    [[nodiscard]] auto layout() -> NamedLayoutSpec &;
    void validate() const;

private:
    StringCharReader &_reader;
    NamedFormatDomain _domain;
    FormatSpec _spec;
    const NamedKeyEntry *_entry{};
};

}
