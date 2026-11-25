// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IdnaData.hpp"

#include "../unicode/UcdReader.hpp"
#include "../unicode/UnicodeData.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringFormat.hpp>

#include <utility>

namespace tool::idna {

using namespace el::text::literals;

IdnaData::IdnaData(const unicode::UcdDataset &dataset) : IdnaData{dataset, unicode::UnicodeData{dataset}} {
}

IdnaData::IdnaData(const unicode::UcdDataset &dataset, const unicode::UnicodeData &unicodeData) :
    _properties{el::ItemCount{cCodePointCount}, 0U} {
    readUnicodeData(dataset, unicodeData);
    applyScripts(dataset, "Scripts.txt"_el);
    applyJoiningTypes(dataset, "DerivedJoiningType.txt"_el);
    applyProperty(dataset, "DerivedNormalizationProps.txt"_el, "Changes_When_NFKC_Casefolded"_el, cUnstable);
    applyProperty(dataset, "DerivedCoreProperties.txt"_el, "Default_Ignorable_Code_Point"_el, cIgnorable);
    applyProperty(dataset, "PropList.txt"_el, "White_Space"_el, cIgnorable);
    applyProperty(dataset, "PropList.txt"_el, "Noncharacter_Code_Point"_el, cIgnorable | cNoncharacter);
    const auto blocks = el::StringList{
        "Combining Diacritical Marks for Symbols"_el,
        "Musical Symbols"_el,
        "Ancient Greek Musical Notation"_el,
    };
    for (const auto &block : blocks) {
        applyProperty(dataset, "Blocks.txt"_el, block, cIgnorableBlock);
    }
    for (const auto &type : el::StringList{"L"_el, "V"_el, "T"_el}) {
        applyProperty(dataset, "HangulSyllableType.txt"_el, type, cOldHangul);
    }
    for (auto codePoint = 0U; codePoint < cCodePointCount; ++codePoint) {
        const auto index = el::ItemIndex::fromInteger(codePoint);
        const auto properties = _properties.get(index);
        const auto status = deriveStatus(static_cast<char32_t>(codePoint), properties);
        _properties.set(index, status == 0 ? 0U : (properties & 0x1FFCU) | status);
    }
}

void IdnaData::addBits(const char32_t first, const char32_t last, const unsigned bits) {
    for (auto codePoint = first; codePoint <= last; ++codePoint) {
        const auto index = el::ItemIndex::fromInteger(static_cast<unsigned>(codePoint));
        _properties.set(index, _properties.get(index) | bits);
    }
}

void IdnaData::readUnicodeData(const unicode::UcdDataset &dataset, const unicode::UnicodeData &unicodeData) {
    const auto source = dataset.sourcePath("UnicodeData.txt"_el).toString();
    for (const auto &record : unicodeData.records()) {
        applyUnicodeRecord(record, record.first, record.last, source);
    }
}

void IdnaData::applyUnicodeRecord(
    const unicode::UcdRecord &record, const char32_t first, const char32_t last, const el::String &source) {
    const auto category = record.fields.get(el::ItemIndex{2});
    // clang-format off
    static const auto categories = el::StringList{
        "Lu"_el, "Ll"_el, "Lt"_el, "Lm"_el, "Lo"_el, "Mn"_el, "Mc"_el, "Me"_el, "Nd"_el, "Nl"_el, "No"_el, "Pc"_el,
        "Pd"_el, "Ps"_el, "Pe"_el, "Pi"_el, "Pf"_el, "Po"_el, "Sm"_el, "Sc"_el, "Sk"_el, "So"_el, "Zs"_el, "Zl"_el,
        "Zp"_el, "Cc"_el, "Cf"_el, "Cs"_el, "Co"_el, "Cn"_el};
    static const auto letterDigits = el::StringList{"Ll"_el, "Lu"_el, "Lo"_el, "Nd"_el, "Lm"_el, "Mn"_el, "Mc"_el};
    static const auto bidiNames = el::StringList{
        ""_el, "L"_el, "R"_el, "AL"_el, "EN"_el, "AN"_el, "ES"_el, "CS"_el, "ET"_el, "ON"_el, "BN"_el, "NSM"_el};
    static const auto otherBidiNames = el::StringList{
        "B"_el, "S"_el, "WS"_el, "LRE"_el, "LRO"_el, "RLE"_el, "RLO"_el, "PDF"_el, "LRI"_el, "RLI"_el, "FSI"_el,
        "PDI"_el};
    // clang-format on
    if (!categories.contains(category)) {
        throwDataError(
            el::StringFormat{"The general category '{}' is not a recognized UCD category."_el}.build(category),
            source,
            record.lineIndex);
    }
    auto bits = category == "Cn"_el ? 0U : cAssigned;
    if (letterDigits.contains(category)) {
        bits |= cLetterDigits;
    }
    unsigned combining;
    try {
        combining = record.fields.get(el::ItemIndex{3})
                        .toIntegerOrThrow<unsigned>(el::IntegerParseOptions{}.setFixedBase(el::IntegerBase::Decimal));
    } catch (const el::ParseError &) {
        throwDataError(
            "The canonical combining class must be a decimal integer between 0 and 255."_el,
            source,
            record.lineIndex,
            std::current_exception());
    }
    if (combining > 255) {
        throwDataError("The canonical combining class exceeds the maximum value of 255."_el, source, record.lineIndex);
    }
    if (combining == 9) {
        bits |= 1U << 9U;
    }
    const auto bidi = record.fields.get(el::ItemIndex{4});
    const auto bidiIndex = bidiNames.findFirst(bidi);
    if (bidiIndex.isValid() && bidiIndex != el::ItemIndex{}) {
        bits |= static_cast<unsigned>(bidiIndex.toSizeT()) << 2U;
    } else if (!otherBidiNames.contains(bidi)) {
        throwDataError(
            el::StringFormat{"The bidi class '{}' is not a recognized UCD bidi class."_el}.build(bidi),
            source,
            record.lineIndex);
    }
    addBits(first, last, bits);
}

void IdnaData::applyProperty(
    const unicode::UcdDataset &dataset, const el::String &filename, const el::String &property, const unsigned bits) {
    auto found = false;
    auto text = dataset.text(filename);
    const auto source = dataset.sourcePath(filename).toString();
    for (const auto &record : unicode::UcdReader{text, source}.parse(el::ItemCount{2})) {
        if (record.fields.get(el::ItemIndex{1}) == property) {
            addBits(record.first, record.last, bits);
            found = true;
        }
    }
    if (!found) {
        throwDataError(
            el::StringFormat{"The source does not contain the required UCD property: {}."_el}.build(property), source);
    }
}

void IdnaData::applyScripts(const unicode::UcdDataset &dataset, const el::String &filename) {
    static const auto scripts = el::StringList{""_el, "Greek"_el, "Han"_el, "Hebrew"_el, "Hiragana"_el, "Katakana"_el};
    auto text = dataset.text(filename);
    const auto source = dataset.sourcePath(filename).toString();
    for (const auto &record : unicode::UcdReader{text, source}.parse(el::ItemCount{2}, true)) {
        const auto scriptIndex = scripts.findFirst(record.fields.get(el::ItemIndex{1}), el::ItemIndex{1});
        if (scriptIndex.isValid()) {
            addBits(record.first, record.last, static_cast<unsigned>(scriptIndex.toSizeT()) << 10U);
        }
    }
}

void IdnaData::applyJoiningTypes(const unicode::UcdDataset &dataset, const el::String &filename) {
    static const auto types = el::StringList{"U"_el, "L"_el, "R"_el, "D"_el, "T"_el, "C"_el};
    auto text = dataset.text(filename);
    const auto source = dataset.sourcePath(filename).toString();
    for (const auto &record : unicode::UcdReader{text, source}.parse(el::ItemCount{2}, true)) {
        const auto name = record.fields.get(el::ItemIndex{1});
        const auto typeIndex = types.findFirst(name);
        if (!typeIndex.isValid()) {
            throwDataError(
                el::StringFormat{"The joining type '{}' is not a recognized UCD joining type."_el}.build(name),
                source,
                record.lineIndex);
        }
        if (typeIndex >= el::ItemIndex{1} && typeIndex <= el::ItemIndex{4}) {
            addBits(record.first, record.last, static_cast<unsigned>(typeIndex.toSizeT()) << 6U);
        }
    }
}

auto IdnaData::deriveStatus(const char32_t codePoint, const unsigned properties) -> unsigned {
    // clang-format off
    switch (codePoint) {
    case 0x00DF: case 0x03C2: case 0x06FD: case 0x06FE: case 0x0F0B: case 0x3007:
        return 1;
    case 0x00B7: case 0x0375: case 0x05F3: case 0x05F4: case 0x30FB:
        return 3;
    case 0x0640: case 0x07FA: case 0x302E: case 0x302F: case 0x3031: case 0x3032: case 0x3033: case 0x3034:
    case 0x3035: case 0x303B:
        return 0;
    default:
        break;
    }
    // clang-format on
    if ((codePoint >= 0x0660 && codePoint <= 0x0669) || (codePoint >= 0x06F0 && codePoint <= 0x06F9)) {
        return 3;
    }
    // RFC 5892 BackwardCompatible is empty; RFC 6452 introduced no overrides.
    if ((properties & cAssigned) == 0 && (properties & cNoncharacter) == 0) {
        return 0;
    }
    if (codePoint == U'-' || (codePoint >= U'0' && codePoint <= U'9') || (codePoint >= U'a' && codePoint <= U'z')) {
        return 1;
    }
    if (codePoint == 0x200C || codePoint == 0x200D) {
        return 2;
    }
    // NFKC_CF additionally removes default ignorables, already disallowed below unless an earlier rule applies.
    if ((properties & (cUnstable | cIgnorable | cIgnorableBlock | cOldHangul)) != 0) {
        return 0;
    }
    return (properties & cLetterDigits) != 0 ? 1U : 0U;
}

auto IdnaData::attributes(const char32_t codePoint) const -> unsigned {
    return codePoint < cCodePointCount ? _properties.get(el::ItemIndex::fromInteger(static_cast<unsigned>(codePoint)))
                                       : 0U;
}

auto IdnaData::ranges() const -> el::List<IdnaRange> {
    auto result = el::List<IdnaRange>{};
    auto codePoint = 0U;
    while (codePoint < cCodePointCount) {
        const auto value = attributes(static_cast<char32_t>(codePoint));
        if (value == 0) {
            ++codePoint;
            continue;
        }
        const auto first = codePoint++;
        while (
            codePoint < cCodePointCount && (first >> 16U) == (codePoint >> 16U) &&
            attributes(static_cast<char32_t>(codePoint)) == value) {
            ++codePoint;
        }
        if (value > 0x1FFF || (value & 3U) == 0) {
            throwDataError("The encoded IDNA attributes exceed the compact representation or have no valid status."_el);
        }
        result.append(IdnaRange{static_cast<char32_t>(first), static_cast<char32_t>(codePoint - 1U), value});
    }
    return result;
}

auto IdnaData::pageOffsets(const el::List<IdnaRange> &ranges) -> el::List<el::ItemIndex> {
    if (ranges.count() > el::ItemCount{0xFFFF}) {
        throwDataError("The IDNA range count exceeds the maximum representable 16-bit page offset."_el);
    }
    auto result = el::List<el::ItemIndex>{};
    if (ranges.isEmpty()) {
        result.append(el::ItemIndex::zero());
        return result;
    }
    const auto pageCount = (ranges.last().last >> 16U) + 1U;
    auto rangeIndex = el::ItemIndex::zero();
    for (auto page = 0U; page < pageCount; ++page) {
        result.append(rangeIndex);
        while (rangeIndex.isWithin(ranges.count()) && (ranges.get(rangeIndex).first >> 16U) == page) {
            rangeIndex.uncheckedIncrement();
        }
    }
    result.append(rangeIndex);
    return result;
}

void IdnaData::throwDataError(
    el::String description,
    const el::String &filename,
    const el::LineIndex lineIndex,
    const std::exception_ptr &cause) {
    auto error = el::ApplicationError{"Failed to Derive IDNA Data"_el, std::move(description), cause};
    if (!filename.isEmpty()) {
        error = error.withSourcePath(filename);
        if (lineIndex.isValid()) {
            error = error.withCodeLocation(el::CodeLocation{lineIndex});
        }
    }
    throw error;
}

}
