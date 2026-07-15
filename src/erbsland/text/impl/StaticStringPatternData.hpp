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

template <std::size_t tMaxElements, std::size_t tMaxRanges>
class StaticStringPatternData final : public StringPatternData {
public:
    StaticStringPatternData() = default;
    template <pattern::AnyElement... Args>
    explicit StaticStringPatternData(const Args &...elements);

public: // implement StringPatternData
    [[nodiscard]] auto view() const noexcept -> StringPatternView override;

private:
    void append(const pattern::Text &text);
    void append(const pattern::OneChar &);
    void append(const pattern::Range &range);
    void append(const pattern::Set &set);
    void append(const pattern::Divider &);
    void appendCharacter(Char character);
    void appendRange(CharRange range);
    void appendDivider();
    void validate() const;
    void requirePattern(bool condition, std::string_view reason) const;
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
