// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPatternData.hpp"

#include "../CharSet_fwd.hpp"
#include "../StringCharReader.hpp"

#include <vector>

namespace erbsland::text::impl {

/// Compiles dynamically parsed string-pattern elements into a compact representation.
/// @notest{Private implementation detail exercised through the string-pattern API.}
class DynamicStringPatternData final : public StringPatternData {
public:
    /// Create empty dynamic pattern data.
    DynamicStringPatternData() = default;

public:
    /// Parse pattern data from a character reader.
    [[nodiscard]] static auto parse(StringCharReader &reader) -> StringPatternDataPtr;

public: // implement StringPatternData
    [[nodiscard]] auto view() const noexcept -> StringPatternView override;

private:
    /// Append a pattern element that matches one arbitrary character.
    void appendOneChar();
    /// Append a pattern element that matches one character.
    void appendCharacter(Char character);
    /// Append a pattern element that matches a character set.
    void appendSet(const CharSet &charSet);
    /// Append a character-set pattern encoded as text.
    void appendSetPattern(const U32String &pattern);
    /// Append the optional pattern divider.
    void appendDivider();
    /// Append an escaped literal character.
    void appendEscapedCharacter(Char character);
    /// Validate the completed pattern representation.
    void validate() const;
    /// Convert a range count to the stored representation or throw on overflow.
    [[nodiscard]] auto checkedRangeCount(std::size_t count) const -> std::uint16_t;

private:
    std::vector<StringPatternElement> _elements;   ///< The compiled pattern elements.
    std::vector<CharRange> _ranges;                ///< The pooled character set ranges.
    std::size_t _divider{cNoStringPatternDivider}; ///< The optional `*` divider position.
};

}
