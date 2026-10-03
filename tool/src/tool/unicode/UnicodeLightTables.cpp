// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeLightTables.hpp"

#include "UcdReader.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSplitter.hpp>

#include <map>
#include <tuple>

namespace tool::unicode {

using namespace el::text;
using namespace el::text::literals;

UnicodeLightTables::UnicodeLightTables(const UnicodeSource &source) {
    constexpr auto cLimit = 0x110000U;
    auto categories = el::StringList{el::ItemCount{cLimit}, "Cn"_el};
    auto mappings = std::vector<LightDelta>(cLimit);
    auto widths = std::vector<uint8_t>(cLimit, 1U);

    for (const auto &record : source.unicodeData().records()) {
        const auto category = record.fields.get(el::ItemIndex{2});
        const auto upperText = record.fields.get(el::ItemIndex{12});
        const auto lowerText = record.fields.get(el::ItemIndex{13});
        const auto upper = upperText.isEmpty() ? 0U : parseHex(upperText);
        const auto lower = lowerText.isEmpty() ? 0U : parseHex(lowerText);
        const auto zeroWidth = category.startsWith("M"_el) || category == "Cc"_el || category == "Cf"_el;
        for (
            auto codePoint = static_cast<uint32_t>(record.first); codePoint <= static_cast<uint32_t>(record.last);
            ++codePoint) {
            categories.set(el::ItemIndex::fromInteger(codePoint), category);
            if (upper != 0U) {
                mappings[codePoint].upper = static_cast<int32_t>(upper) - static_cast<int32_t>(codePoint);
            }
            if (lower != 0U) {
                mappings[codePoint].lower = static_cast<int32_t>(lower) - static_cast<int32_t>(codePoint);
            }
            if (zeroWidth) {
                widths[codePoint] = 0U;
            }
        }
    }
    const auto &dataset = source.dataset();
    for (
        const auto &record :
        UcdReader{dataset.text("EastAsianWidth.txt"_el), dataset.sourcePath("EastAsianWidth.txt"_el).toString()}.parse(
            el::ItemCount{2})) {
        const auto widthClass = record.fields.get(el::ItemIndex{1});
        if (widthClass != "W"_el && widthClass != "F"_el) {
            continue;
        }
        for (
            auto codePoint = static_cast<uint32_t>(record.first); codePoint <= static_cast<uint32_t>(record.last);
            ++codePoint) {
            widths[codePoint] = 2U;
        }
    }
    // General categories take precedence over the East Asian width class.
    for (const auto &record : source.unicodeData().records()) {
        const auto category = record.fields.get(el::ItemIndex{2});
        if (!(category.startsWith("M"_el) || category == "Cc"_el || category == "Cf"_el)) {
            continue;
        }
        for (
            auto codePoint = static_cast<uint32_t>(record.first); codePoint <= static_cast<uint32_t>(record.last);
            ++codePoint) {
            widths[codePoint] = 0U;
        }
    }
    for (
        const auto &record :
        UcdReader{dataset.text("CaseFolding.txt"_el), dataset.sourcePath("CaseFolding.txt"_el).toString()}.parse(
            el::ItemCount{3})) {
        const auto status = record.fields.get(el::ItemIndex{1});
        if (status != "C"_el && status != "S"_el) {
            continue;
        }
        const auto mapping = record.fields.get(el::ItemIndex{2});
        if (mapping.isEmpty() || mapping.contains(" "_el) || record.first != record.last) {
            continue;
        }
        const auto destination = parseHex(mapping);
        const auto codePoint = static_cast<uint32_t>(record.first);
        mappings[codePoint].fold = static_cast<int32_t>(destination) - static_cast<int32_t>(codePoint);
    }

    auto indexes = std::map<std::tuple<int32_t, int32_t, int32_t>, uint8_t>{};
    _deltas.push_back({});
    indexes.emplace(std::tuple{0, 0, 0}, 0U);
    auto previous = LightRange{};
    for (auto codePoint = 0U; codePoint < cLimit; ++codePoint) {
        const auto &mapping = mappings[codePoint];
        const auto key = std::tuple{mapping.fold, mapping.lower, mapping.upper};
        auto it = indexes.find(key);
        if (it == indexes.end()) {
            if (_deltas.size() >= 256U) {
                throw el::ApplicationError{
                    "Unicode Light Data Exceeds Runtime Limit"_el,
                    "The case delta table requires more than 256 entries."_el};
            }
            const auto index = static_cast<uint8_t>(_deltas.size());
            _deltas.push_back(mapping);
            it = indexes.emplace(key, index).first;
        }
        const auto &category = categories.getRef(el::ItemIndex::fromInteger(codePoint));
        if (codePoint == 0U || category != previous.category || it->second != previous.deltaIndex ||
            widths[codePoint] != previous.width) {
            previous = LightRange{codePoint, category, it->second, widths[codePoint]};
            _ranges.push_back(previous);
        }
    }
    _ranges.push_back(LightRange{cLimit, "Cn"_el});
    if (_ranges.size() > 128U + 4451U) {
        throw el::ApplicationError{
            "Unicode Light Data Exceeds Runtime Limit"_el,
            "The compressed Unicode range table exceeds its size budget."_el};
    }
}

auto UnicodeLightTables::parseHex(const el::String &text) -> uint32_t {
    auto options = el::IntegerParseOptions{}
                       .setFixedBase(el::IntegerBase::Hexadecimal)
                       .setMinimumDigits(el::CpCount{1})
                       .setMaximumDigits(el::CpCount{6})
                       .setFlags(el::IntegerParseFlags{});
    const auto value = text.toIntegerOrThrow<uint32_t>(options);
    if (value > 0x10FFFFU) {
        throw el::ApplicationError{"Invalid UCD Code Point"_el, "A code point exceeds U+10FFFF."_el};
    }
    return value;
}

auto UnicodeLightTables::categoryName(const el::String &category) -> el::String {
    static const auto names = el::StringMap<el::String>{
        {"Cc"_el, "Control"_el},
        {"Cf"_el, "Format"_el},
        {"Cn"_el, "Unassigned"_el},
        {"Co"_el, "PrivateUse"_el},
        {"Cs"_el, "Surrogate"_el},
        {"Ll"_el, "LowercaseLetter"_el},
        {"Lm"_el, "ModifierLetter"_el},
        {"Lo"_el, "OtherLetter"_el},
        {"Lt"_el, "TitlecaseLetter"_el},
        {"Lu"_el, "UppercaseLetter"_el},
        {"Mc"_el, "SpacingMark"_el},
        {"Me"_el, "EnclosingMark"_el},
        {"Mn"_el, "NonspacingMark"_el},
        {"Nd"_el, "DecimalNumber"_el},
        {"Nl"_el, "LetterNumber"_el},
        {"No"_el, "OtherNumber"_el},
        {"Pc"_el, "ConnectorPunctuation"_el},
        {"Pd"_el, "DashPunctuation"_el},
        {"Pe"_el, "ClosePunctuation"_el},
        {"Pf"_el, "FinalPunctuation"_el},
        {"Pi"_el, "InitialPunctuation"_el},
        {"Po"_el, "OtherPunctuation"_el},
        {"Ps"_el, "OpenPunctuation"_el},
        {"Sc"_el, "CurrencySymbol"_el},
        {"Sk"_el, "ModifierSymbol"_el},
        {"Sm"_el, "MathSymbol"_el},
        {"So"_el, "OtherSymbol"_el},
        {"Zl"_el, "LineSeparator"_el},
        {"Zp"_el, "ParagraphSeparator"_el},
        {"Zs"_el, "SpaceSeparator"_el}};
    const auto name = names.get(category, el::String{});
    if (name.isEmpty()) {
        throw el::ApplicationError{"Invalid Unicode Category"_el, "Unknown UCD general category."_el};
    }
    return name;
}

auto UnicodeLightTables::entryAt(const uint32_t codePoint) const -> LightRange {
    auto result = _ranges.front();
    for (const auto &entry : _ranges) {
        if (entry.start > codePoint) {
            break;
        }
        result = entry;
    }
    result.start = codePoint;
    return result;
}

auto UnicodeLightTables::renderData() const -> el::String {
    const auto startFormat = el::StringFormat{"0x{:number:hex,precision=6,lc=uppercase}U"_el};
    const auto deltaFormat = el::StringFormat{"0x{:number:hex,precision=2,lc=uppercase}U"_el};
    const auto prepare = [this, &startFormat, &deltaFormat](const LightRange &entry) {
        auto result = render::ValueMap{};
        result.set("start"_el, startFormat.build(entry.start));
        result.set("category"_el, categoryName(entry.category));
        result.set("delta"_el, deltaFormat.build(entry.deltaIndex));
        result.set("width"_el, entry.width);
        result.set("custom"_el, entry.deltaIndex != 0U || entry.width != 1U);
        return result;
    };
    auto ascii = render::ValueList{};
    for (auto codePoint = 0U; codePoint < 128U; ++codePoint) {
        ascii.append(prepare(entryAt(codePoint)));
    }
    auto ranges = render::ValueList{};
    auto first = _ranges.begin();
    while (first != _ranges.end() && first->start < 128U) {
        ++first;
    }
    const auto count = static_cast<std::size_t>(_ranges.end() - first) + (first->start == 128U ? 0U : 1U);
    if (first->start != 128U) {
        ranges.append(prepare(entryAt(128U)));
    }
    for (auto it = first; it != _ranges.end(); ++it) {
        ranges.append(prepare(*it));
    }
    auto deltas = render::ValueList{};
    const auto numberFormat = el::StringFormat{"{}"_el};
    for (const auto &delta : _deltas) {
        deltas.append(
            el::join(
                "UnicodeDelta{"_el,
                numberFormat.build(delta.fold),
                ", "_el,
                numberFormat.build(delta.lower),
                ", "_el,
                numberFormat.build(delta.upper),
                "},"_el));
    }
    auto context = render::Context{};
    context.set("ascii"_el, ascii);
    context.set("ranges"_el, ranges);
    context.set("range_count"_el, count);
    context.set("deltas"_el, deltas);
    context.set("delta_count"_el, _deltas.size());
    return frame::app()
        .createRenderEnvironment(frame::TemplateLanguage::Cpp)
        ->render("unicode/light_data.cpp"_el, context);
}

auto UnicodeLightTables::renderVersion(const el::String &version) -> el::String {
    auto parts = el::StringList{};
    auto splitter = el::StringSplitter{version, U'.'};
    while (!splitter.isAtEnd()) {
        parts.append(splitter.next());
    }
    if (parts.count() != el::ItemCount{3}) {
        throw el::ApplicationError{"Invalid Unicode Version"_el, "Expected three version components."_el};
    }
    auto context = render::Context{};
    context.set("major"_el, parts.get(el::ItemIndex{0}));
    context.set("minor"_el, parts.get(el::ItemIndex{1}));
    context.set("patch"_el, parts.get(el::ItemIndex{2}));
    return frame::app()
        .createRenderEnvironment(frame::TemplateLanguage::Cpp)
        ->render("unicode/version.cpp"_el, context);
}

}
