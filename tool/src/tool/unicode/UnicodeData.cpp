// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeData.hpp"

#include "UcdReader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CharRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/ParseError.hpp>

namespace tool::unicode {

using namespace el::text::literals;

UnicodeData::UnicodeData(const UcdDataset &dataset) : _source{dataset.sourcePath("UnicodeData.txt"_el).toString()} {
    auto pending = UcdRecord{};
    auto inRange = false;
    auto next = 0U;
    for (const auto &record : UcdReader{dataset.text("UnicodeData.txt"_el), _source}.parse(el::ItemCount{15}, true)) {
        validateProperties(record);
        if (record.first != record.last || static_cast<unsigned>(record.first) < next) {
            throwError(record, "UnicodeData entries must be unique, sorted single code points."_el);
        }
        next = static_cast<unsigned>(record.first) + 1U;
        appendRecord(record, pending, inRange);
    }
    if (inRange || next == 0U) {
        throwError(pending, "Empty or unterminated UnicodeData input."_el);
    }
}

void UnicodeData::appendRecord(UcdRecord record, UcdRecord &pending, bool &inRange) {
    const auto name = record.fields.get(el::ItemIndex{1});
    if (name.endsWith(", First>"_el)) {
        if (inRange) {
            throwError(record, "A range start precedes the end of an earlier UnicodeData range."_el);
        }
        pending = record;
        inRange = true;
    } else if (name.endsWith(", Last>"_el)) {
        if (!inRange || pending.fields.get(el::ItemIndex{1}).replacedAll("First>"_el, "Last>"_el) != name) {
            throwError(record, "UnicodeData range end has no matching start."_el);
        }
        validateRange(pending, record);
        record.first = pending.first;
        record.lineIndex = pending.lineIndex;
        _records.append(record);
        inRange = false;
    } else {
        if (inRange) {
            throwError(record, "A regular record interrupts a UnicodeData range."_el);
        }
        _records.append(record);
    }
}

void UnicodeData::validateProperties(const UcdRecord &record) const {
    static const auto categories = el::StringList{
        "Lu"_el,
        "Ll"_el,
        "Lt"_el,
        "Lm"_el,
        "Lo"_el,
        "Mn"_el,
        "Mc"_el,
        "Me"_el,
        "Nd"_el,
        "Nl"_el,
        "No"_el,
        "Pc"_el,
        "Pd"_el,
        "Ps"_el,
        "Pe"_el,
        "Pi"_el,
        "Pf"_el,
        "Po"_el,
        "Sm"_el,
        "Sc"_el,
        "Sk"_el,
        "So"_el,
        "Zs"_el,
        "Zl"_el,
        "Zp"_el,
        "Cc"_el,
        "Cf"_el,
        "Cs"_el,
        "Co"_el,
        "Cn"_el};
    static const auto directions = el::StringList{
        "L"_el,
        "R"_el,
        "AL"_el,
        "EN"_el,
        "ES"_el,
        "ET"_el,
        "AN"_el,
        "CS"_el,
        "NSM"_el,
        "BN"_el,
        "B"_el,
        "S"_el,
        "WS"_el,
        "ON"_el,
        "LRE"_el,
        "LRO"_el,
        "RLE"_el,
        "RLO"_el,
        "PDF"_el,
        "LRI"_el,
        "RLI"_el,
        "FSI"_el,
        "PDI"_el};
    if (!categories.contains(record.fields.get(el::ItemIndex{2})) ||
        !directions.contains(record.fields.get(el::ItemIndex{4}))) {
        throwError(record, "Unknown UnicodeData category or bidi class."_el);
    }
    const auto combining = record.fields.get(el::ItemIndex{3});
    try {
        if (combining.isEmpty() || combining.findFirstNotOf(el::CharSet{el::CharRange{U'0', U'9'}}).isValid()) {
            throw el::ParseError{"A canonical combining class must use ASCII decimal syntax."_el};
        }
        if (combining.toIntegerOrThrow<unsigned>() > 255U) {
            throwError(record, "A canonical combining class exceeds 255."_el);
        }
    } catch (const el::ParseError &) {
        throw el::ApplicationError{
            "Invalid UnicodeData Record"_el,
            "Failed to parse a canonical combining class."_el,
            std::current_exception()}
            .withSourcePath(_source)
            .withCodeLocation(el::CodeLocation{record.lineIndex});
    }
}

void UnicodeData::validateRange(const UcdRecord &first, const UcdRecord &last) const {
    for (auto index = el::ItemIndex{2U}; index.isWithin(el::ItemCount{15U}); index.uncheckedIncrement()) {
        if (first.fields.get(index) != last.fields.get(index)) {
            throwError(last, "Properties differ between UnicodeData range endpoints."_el);
        }
    }
}

void UnicodeData::throwError(const UcdRecord &record, const el::String &description) const {
    throw el::ApplicationError{"Invalid UnicodeData Record"_el, description}.withSourcePath(_source).withCodeLocation(
        el::CodeLocation{record.lineIndex});
}

}
