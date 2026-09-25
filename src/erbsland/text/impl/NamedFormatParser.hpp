// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatSpec.hpp"
#include "NamedFormatDomain.hpp"

#include "../named_key/Entry.hpp"
#include "../named_key/Format.hpp"
#include "../StringCharReader.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::text::impl {

/// Parser for typed named format specifications.
/// @tested{U8FormatTest U16FormatTest U32FormatTest}
class NamedFormatParser final {
    /// Identifies one supported named format option.
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
    /// Get the named-key format accepted by this parser.
    [[nodiscard]] static auto namedKeyFormat() -> const named_key::Format &;
    /// Convert a format-domain name to its domain.
    [[nodiscard]] static auto domainFromName(const String &name) -> std::optional<NamedFormatDomain>;
    /// Test whether an option is valid in a format domain.
    [[nodiscard]] static auto isAllowed(Option option, NamedFormatDomain domain) noexcept -> bool;
    /// Read the selected option value as a normalized identifier.
    [[nodiscard]] auto readIdentifier() const -> String;
    /// Read the selected option value as a decimal number.
    [[nodiscard]] auto readNumber() -> unit::CpLength;
    /// Read the selected option value as one character.
    [[nodiscard]] auto readFill() const -> Char;
    /// Parse one named option entry.
    void parseOption(Option option, const named_key::Entry &entry);
    /// Require that the selected option has no value.
    void requireKeyWithoutValue() const;
    /// Parse the layout width option.
    void parseWidth();
    /// Parse the layout alignment option.
    void parseAlignment();
    /// Parse the layout fill option.
    void parseFill();
    /// Parse the maximum text or byte length option.
    void parseMaximum();
    /// Parse the text escaping option.
    void parseEscape();
    /// Parse the text escape amount option.
    void parseEscapeAmount();
    /// Parse the number base option.
    void parseBase();
    /// Parse the number notation option.
    void parseNotation();
    /// Parse the number letter-case option.
    void parseLetterCase();
    /// Parse the number sign option.
    void parseSign();
    /// Parse the number precision option.
    void parsePrecision();
    /// Parse the number alternate-form flag.
    void parseAlternate();
    /// Parse the number zero-fill flag.
    void parseZeroFill();
    /// Parse the Boolean rendering style option.
    void parseStyle();
    /// Parse the Boolean capitalization option.
    void parseCapitalization();
    /// Parse the byte separator option.
    void parseSeparator();
    /// Parse the byte truncation flag.
    void parseTruncate();
    /// Get the active format layout specification.
    [[nodiscard]] auto layout() -> NamedLayoutSpec &;
    /// Validate the completed format specification.
    void validate() const;

private:
    StringCharReader &_reader;
    NamedFormatDomain _domain;
    FormatSpec _spec;
    const named_key::Entry *_entry{};
};

}
