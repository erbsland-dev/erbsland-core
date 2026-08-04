// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPatternData.hpp"

#include "../pattern/AnyElement.hpp"
#include "../pattern/Divider.hpp"
#include "../pattern/impl/Require.hpp"
#include "../pattern/OneChar.hpp"
#include "../pattern/PatternSet.hpp"
#include "../pattern/Range.hpp"
#include "../pattern/Text.hpp"

#include <array>
#include <limits>
#include <string_view>

namespace erbsland::text::impl {

/// Compile a bounded string pattern into fixed-capacity storage.
/// @tparam tMaxElements The maximum number of generated pattern elements.
/// @tparam tMaxRanges The maximum number of stored character ranges.
/// @tested{StringPatternTest}
template <std::size_t tMaxElements, std::size_t tMaxRanges>
class StaticStringPatternData final : public StringPatternData {
public:
    /// Create an empty compiled pattern.
    StaticStringPatternData() = default;
    /// Compile a sequence of string pattern elements.
    /// @tparam Args The supplied pattern element types.
    /// @param elements The pattern elements to compile.
    template <pattern::AnyElement... Args>
    explicit StaticStringPatternData(const Args &...elements);

public: // implement StringPatternData
    [[nodiscard]] auto view() const noexcept -> StringPatternView override;

private:
    /// Append literal text as compiled pattern elements.
    /// @param text The literal text element.
    void append(const pattern::Text &text);
    /// Append a single-character pattern element.
    void append(const pattern::OneChar &);
    /// Append a character-range pattern element.
    /// @param range The range to append.
    void append(const pattern::Range &range);
    /// Append a character-set pattern element.
    /// @param set The set to append.
    void append(const pattern::Set &set);
    /// Append the pattern divider element.
    void append(const pattern::Divider &);
    /// Append one literal character.
    /// @param character The character to append.
    void appendCharacter(Char character);
    /// Append one character range to the shared range pool.
    /// @param range The range to append.
    void appendRange(CharRange range);
    /// Append the compiled divider marker.
    void appendDivider();
    /// Validate the compiled pattern's structural invariants.
    void validate() const;
    /// Require a pattern construction condition.
    /// @param condition The condition to require.
    /// @param reason The reason reported when the condition is false.
    void requirePattern(bool condition, std::string_view reason) const;
    /// Convert and bounds-check a range count.
    /// @param count The count to convert.
    /// @return The count represented as `uint16_t`.
    [[nodiscard]] auto checkedRangeCount(std::size_t count) const -> std::uint16_t;

private:
    std::array<StringPatternElement, tMaxElements> _elements{}; ///< The compiled pattern elements.
    std::array<CharRange, tMaxRanges> _ranges{};                ///< The pooled character set ranges.
    std::size_t _elementCount{};                                ///< The number of used elements.
    std::size_t _rangeCount{};                                  ///< The number of used ranges.
    std::size_t _divider{cNoStringPatternDivider};              ///< The optional `*` divider position.
};

}

#include "StaticStringPatternData.tpp"
