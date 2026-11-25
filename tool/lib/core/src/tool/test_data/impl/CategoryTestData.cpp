// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CategoryTestData.hpp"

#include <erbsland/Set.hpp>
#include <erbsland/StringFormat.hpp>

#include <array>

namespace tool::test_data::impl {

using namespace el::text::literals;

auto CategoryTestData::render(const el::List<unicode::UcdRecord> &records, const el::String &version) -> el::String {
    auto data = CategoryTestData{};
    data._output.append(
        el::StringFormat{
            "# CategoryContainsTest data\n"
            "# Generated from UnicodeData.txt (Unicode {}) by erbsland-core-tool generate-category-test-data.\n"
            "# Format: HEX_CODE_POINT;MaskExpr (where MaskExpr uses Category::Value enumerator names)\n"_el}
            .build(version));
    auto next = uint32_t{0};
    for (const auto &record : records) {
        const auto first = static_cast<uint32_t>(record.first);
        const auto last = static_cast<uint32_t>(record.last);
        if (next < first) {
            data.appendCategoryRange(next, first - 1, "Cn"_el);
        }
        data.appendCategoryRange(first, last, record.fields.get(el::ItemIndex{2}));
        next = last + 1;
    }
    if (next <= 0x10FFFFU) {
        data.appendCategoryRange(next, 0x10FFFFU, "Cn"_el);
    }
    data.flushRange();
    return data._output;
}

auto CategoryTestData::expression(const uint32_t codePoint, const el::String &category) -> el::String {
    auto names = el::Set<el::String>{category, "AnyDotAll"_el};
    if (category == "Nd"_el) {
        names.insert("DigitUnicode"_el);
    }
    if (category.startsWith("L"_el) || category == "Nd"_el || codePoint == 0x5FU) {
        names.insert("WordUnicode"_el);
    }
    if (category == "Zl"_el || category == "Zp"_el) {
        names.insert("SpaceUnicodeDotAll"_el);
        names.insert("VerticalSpaceUnicode"_el);
    }
    if (category == "Zs"_el) {
        names.insert("SpaceUnicode"_el);
        names.insert("SpaceUnicodeDotAll"_el);
        names.insert("HorizontalSpaceUnicode"_el);
    }
    if (codePoint >= 0x30U && codePoint <= 0x39U) {
        names.insert("DigitAscii"_el);
        names.insert("WordAscii"_el);
    }
    if ((codePoint >= 0x41U && codePoint <= 0x5AU) || (codePoint >= 0x61U && codePoint <= 0x7AU) ||
        codePoint == 0x5FU) {
        names.insert("WordAscii"_el);
    }
    if (codePoint == 0x20U || codePoint == 0x09U) {
        names.insert("SpaceAscii"_el);
        names.insert("SpaceAsciiDotAll"_el);
        names.insert("SpaceUnicode"_el);
        names.insert("SpaceUnicodeDotAll"_el);
        names.insert("HorizontalSpaceAscii"_el);
    }
    if (codePoint == 0xA0U) {
        names.insert("HorizontalSpaceAscii"_el);
    }
    if (codePoint >= 0x0AU && codePoint <= 0x0DU) {
        names.insert("SpaceAsciiDotAll"_el);
        names.insert("SpaceUnicodeDotAll"_el);
        names.insert("VerticalSpaceAscii"_el);
    }
    if ((codePoint >= 0x0AU && codePoint <= 0x0DU) || codePoint == 0x85U) {
        names.insert("VerticalSpaceUnicode"_el);
    }
    if (codePoint != 0x0AU) {
        names.insert("Any"_el);
    }
    auto result = el::StringEditor{};
    for (const auto &name : names) {
        if (!result.isEmpty()) {
            result.append(" | "_el);
        }
        result.append(name);
    }
    return result;
}

void CategoryTestData::appendCategoryRange(uint32_t first, const uint32_t last, const el::String &category) {
    // These boundaries cover every code-point-dependent rule in expression().
    constexpr auto cBoundaries = std::array<uint32_t, 18>{
        0x09U,
        0x0AU,
        0x0BU,
        0x0EU,
        0x20U,
        0x21U,
        0x30U,
        0x3AU,
        0x41U,
        0x5BU,
        0x5FU,
        0x60U,
        0x61U,
        0x7BU,
        0x85U,
        0x86U,
        0xA0U,
        0xA1U};
    for (const auto boundary : cBoundaries) {
        if (boundary > last) {
            break;
        }
        if (boundary > first) {
            appendRange(first, boundary - 1, expression(first, category));
            first = boundary;
        }
    }
    appendRange(first, last, expression(first, category));
}

void CategoryTestData::appendRange(const uint32_t first, const uint32_t last, const el::String &expression) {
    if (!_expression.isEmpty() && _last + 1 == first && _expression == expression) {
        _last = last;
        return;
    }
    flushRange();
    _first = first;
    _last = last;
    _expression = expression;
}

void CategoryTestData::flushRange() {
    if (_expression.isEmpty()) {
        return;
    }
    appendSample(_first);
    if (_last > _first) {
        appendSample(_last);
    }
    if (_last - _first >= 2) {
        appendSample(_first + (_last - _first) / 2);
    }
}

void CategoryTestData::appendSample(const uint32_t codePoint) {
    _output.append(el::StringFormat{"{:number:hex,precision=6,lc=uppercase};{}\n"_el}.build(codePoint, _expression));
}

}
