// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unicode/UcdRecord.hpp"

#include <erbsland/List.hpp>
#include <erbsland/StringEditor.hpp>

#include <cstdint>

namespace tool::test_data::impl {

/// Calculate category fixture expressions without calling the regex category implementation.
class CategoryTestData final {
public: // generation
    /// Render boundary and interior samples for every contiguous category-expression range.
    /// @param records Ordered, validated UnicodeData records, including expanded First/Last ranges.
    /// @param version Verified Unicode snapshot version for the provenance header.
    /// @return The complete fixture; gaps have category Cn, including the tail through U+10FFFF.
    [[nodiscard]] static auto render(const el::List<unicode::UcdRecord> &records, const el::String &version)
        -> el::String;
    /// Calculate the independent expected mask expression for one code point.
    /// @param codePoint A code point from U+0000 through U+10FFFF, including surrogates.
    /// @param category Validated two-letter UCD general category.
    /// @return Deduplicated, alphabetically ordered Category::Value names separated by a vertical bar.
    [[nodiscard]] static auto expression(uint32_t codePoint, const el::String &category) -> el::String;

private:
    /// Split a general-category range at the explicit regex-class boundaries.
    void appendCategoryRange(uint32_t first, uint32_t last, const el::String &category);
    /// Merge consecutive equal expressions, flushing the previous range when necessary.
    void appendRange(uint32_t first, uint32_t last, const el::String &expression);
    /// Emit the pending range's first, last, and midpoint, without duplicate samples.
    void flushRange();
    /// Emit one hexadecimal code point and its mask expression.
    void appendSample(uint32_t codePoint);

private:
    el::StringEditor _output; ///< Complete generated fixture.
    el::String _expression;   ///< Pending range expression; empty before the first range.
    uint32_t _first{};        ///< First pending code point.
    uint32_t _last{};         ///< Last pending code point.
};

}
