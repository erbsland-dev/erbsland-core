// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IdnaMapping.hpp"

#include "UcdReader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U8StringConstIterator.hpp>

namespace tool::unicode {

using namespace el::text::literals;

IdnaMapping::IdnaMapping(const UcdDataset &dataset) :
    _source{dataset.sourcePath("IdnaMappingTable.txt"_el).toString()} {
    auto reader = UcdReader{dataset.text("IdnaMappingTable.txt"_el), _source};
    _records = reader.parse(el::ItemCount{2});
    auto next = 0U;
    for (const auto &record : _records) {
        if (record.first != next) {
            throwError("Mapping ranges must cover Unicode without gaps or overlaps."_el, record.lineIndex);
        }
        next = static_cast<unsigned>(record.last) + 1U;
        const auto status = record.fields.get(el::ItemIndex{1});
        static const auto statuses = el::StringList{
            "valid"_el,
            "mapped"_el,
            "ignored"_el,
            "deviation"_el,
            "disallowed"_el,
            "disallowed_STD3_valid"_el,
            "disallowed_STD3_mapped"_el};
        if (!statuses.contains(status)) {
            throwError("Unknown IDNA mapping status."_el, record.lineIndex);
        }
        const auto replacement = readReplacement(record, reader);
        if ((status == "mapped"_el || status == "disallowed_STD3_mapped"_el) && replacement.isEmpty()) {
            throwError("Mapped characters require a replacement."_el, record.lineIndex);
        }
        if (!replacement.isEmpty() && status != "mapped"_el && status != "deviation"_el &&
            status != "disallowed_STD3_mapped"_el) {
            throwError("This mapping status must not have replacement text."_el, record.lineIndex);
        }
        _replacements.append(replacement);
    }
    if (next != 0x110000U) {
        throwError("Mapping table does not cover all Unicode code points."_el);
    }
}

auto IdnaMapping::readReplacement(const UcdRecord &record, const UcdReader &reader) const -> el::String {
    auto replacement = el::StringEditor{};
    if (record.fields.count() > el::ItemCount{2}) {
        auto split = el::StringSplitter{record.fields.get(el::ItemIndex{2}), U' '};
        while (!split.isAtEnd()) {
            const auto field = split.next();
            if (!field.isEmpty()) {
                const auto value = reader.codePoint(field);
                if (value >= 0xD800U && value <= 0xDFFFU) {
                    throwError("IDNA replacements must contain Unicode scalars."_el, record.lineIndex);
                }
                replacement.append(el::Char{value});
            }
        }
    }
    return replacement;
}

auto IdnaMapping::map(const el::String &text) const -> el::String {
    if (!text.isEncodingValid()) {
        throwError("IDNA input is not valid UTF-8."_el);
    }
    auto result = el::StringEditor{};
    for (const auto value : text) {
        const auto index = mappingIndex(value);
        const auto &record = _records.get(index);
        const auto status = record.fields.get(el::ItemIndex{1});
        if (status == "valid"_el || status == "deviation"_el || status == "disallowed_STD3_valid"_el) {
            result.append(value);
        } else if (status == "mapped"_el || status == "disallowed_STD3_mapped"_el) {
            result.append(_replacements.get(index));
        } else if (status != "ignored"_el) {
            throwError("The input contains a disallowed IDNA mapping character."_el, record.lineIndex);
        }
    }
    return result;
}

auto IdnaMapping::mappingIndex(const el::Char value) const -> el::ItemIndex {
    auto lower = std::size_t{};
    auto upper = _records.count().toSizeT();
    while (lower < upper) {
        const auto middle = lower + (upper - lower) / 2U;
        if (_records.get(el::ItemIndex::fromSizeTOrThrow(middle)).last < value.toRawValue()) {
            lower = middle + 1U;
        } else {
            upper = middle;
        }
    }
    return el::ItemIndex::fromSizeTOrThrow(lower);
}

void IdnaMapping::throwError(const el::String &description, const el::LineIndex line) const {
    throw el::ApplicationError{"IDNA Mapping Failed"_el, description}.withSourcePath(_source).withCodeLocation(
        el::CodeLocation{line});
}

}
