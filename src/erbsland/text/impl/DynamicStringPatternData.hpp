// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPatternData.hpp"
#include "StringPatternParseMode.hpp"

#include "../CharSet_fwd.hpp"
#include "../StringCharReader.hpp"
#include "../StringPatternKind.hpp"

#include <vector>

namespace erbsland::text::impl {

/// Compiles dynamically parsed string-pattern elements into a compact representation.
/// @tested{StringPatternTest PathGlobPatternTest}
class DynamicStringPatternData final : public StringPatternData {
public:
    /// Create empty dynamic pattern data.
    DynamicStringPatternData() = default;

public:
    /// Parse Divider or Erbsland Core Glob pattern data from a character reader.
    /// @param reader Decoded pattern characters.
    /// @param mode Syntax and stopping boundaries. Path delimiters remain unread.
    /// @param trimTrailingWhitespace Remove bare trailing spaces/tabs at the end of a path alternative, not before `/`.
    /// @return Immutable compiled elements and ranges.
    /// @throws ParseError If the syntax is invalid or the range representation overflows.
    [[nodiscard]] static auto parse(
        StringCharReader &reader, StringPatternParseMode mode, bool trimTrailingWhitespace = false)
        -> StringPatternDataPtr;
    /// Compile a full-string, case-sensitive Python-style glob.
    /// @param reader Decoded pattern characters.
    /// @return Immutable compiled elements and ranges.
    /// @throws ParseError If the range representation overflows or a character is invalid.
    [[nodiscard]] static auto parsePosixGlob(StringCharReader &reader) -> StringPatternDataPtr;

public: // implement StringPatternData
    [[nodiscard]] auto view() const noexcept -> StringPatternView override;

private:
    /// Parse elements up to the mode's boundary, trim permitted whitespace, and validate path stars.
    void parseElements(StringCharReader &reader, StringPatternParseMode mode, bool trimTrailingWhitespace);
    /// Parse one element after reading its first character.
    void parseElement(StringCharReader &reader, Char character, StringPatternParseMode mode);
    /// Parse a star after its first asterisk, selecting divider, ordinary, or recursive semantics.
    void parseStar(StringCharReader &reader, StringPatternParseMode mode);
    /// Parse a Core character set after its opening bracket.
    void parseSet(StringCharReader &reader, StringPatternParseMode mode);
    /// Read and validate an escape after its backslash, inside or outside a set.
    [[nodiscard]] static auto readEscapedCharacter(StringCharReader &reader, StringPatternParseMode mode) -> Char;
    /// Test for a boundary left unread by partial path parsing.
    [[nodiscard]] static auto isPathBoundary(Char character) noexcept -> bool;
    /// Require recursive stars to occupy the complete component after permitted trimming.
    void validateRecursiveStar() const;
    /// Compile a bracket class, leaving an unmatched opening bracket as a literal.
    void appendPosixGlobSet(StringCharReader &reader);
    /// Read one bracket-class literal or range directly into compiled range storage.
    void appendPosixGlobRange(StringCharReader &reader);
    /// Append a pattern element that matches one arbitrary character.
    void appendOneChar();
    /// Append a pattern element that matches one character.
    void appendCharacter(Char character);
    /// Append a pattern element that matches a character set.
    void appendSet(const CharSet &charSet);
    /// Append a character-set pattern encoded as text.
    void appendSetPattern(const U32String &pattern);
    /// Append a glob wildcard, coalescing adjacent stars.
    void appendStar();
    /// Append the optional pattern divider.
    void appendDivider();
    /// Validate the completed pattern representation.
    void validate() const;
    /// Convert a range count to the stored representation or throw on overflow.
    [[nodiscard]] auto checkedRangeCount(std::size_t count) const -> std::uint16_t;

private:
    StringPatternKind _kind{StringPatternKind::Divider}; ///< Matching and extraction semantics.
    std::vector<StringPatternElement> _elements;         ///< The compiled pattern elements.
    std::vector<CharRange> _ranges;                      ///< The pooled character set ranges.
    std::size_t _divider{cNoStringPatternDivider};       ///< The optional `*` divider position.
};

}
