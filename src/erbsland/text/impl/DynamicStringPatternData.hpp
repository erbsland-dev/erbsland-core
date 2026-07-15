// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPatternData.hpp"

#include "../CharSet_fwd.hpp"
#include "../StringCharReader.hpp"

#include <vector>

namespace erbsland::text::impl {

class DynamicStringPatternData final : public StringPatternData {
public:
    DynamicStringPatternData() = default;

public:
    [[nodiscard]] static auto parse(StringCharReader &reader) -> StringPatternDataPtr;

public: // implement StringPatternData
    [[nodiscard]] auto view() const noexcept -> StringPatternView override;

private:
    void appendOneChar();
    void appendCharacter(Char character);
    void appendSet(const CharSet &charSet);
    void appendSetPattern(const U32StringView &pattern);
    void appendDivider();
    void appendEscapedCharacter(Char character);
    void validate() const;
    [[nodiscard]] auto checkedRangeCount(std::size_t count) const -> std::uint16_t;

private:
    std::vector<StringPatternElement> _elements;   ///< The compiled pattern elements.
    std::vector<CharRange> _ranges;                ///< The pooled character set ranges.
    std::size_t _divider{cNoStringPatternDivider}; ///< The optional `*` divider position.
};

}
