// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UcdReader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringSplitter.hpp>

#include <utility>

namespace tool::unicode {

using namespace el::text::literals;

UcdReader::UcdReader(el::String text, el::String filename) : _text(std::move(text)), _fileName(std::move(filename)) {
}

auto UcdReader::codePoint(const el::String &text) const -> char32_t {
    try {
        // Prepare options for the integer conversion.
        auto options =
            el::IntegerParseOptions{}
                .setMinimumDigits(el::CpCount{1})           // allow from 1 to 6 digits.
                .setMaximumDigits(el::CpCount{6})
                .setFixedBase(el::IntegerBase::Hexadecimal) // enforce hexadecimal base and reject `0x` prefix.
                .setFlags(el::IntegerParseFlags{});
        // The parser does all the work for us.
        const auto value = text.toIntegerOrThrow<uint32_t>(options);
        if (value > 0x10FFFFU) {
            throwError("A parsed UCD code point exceeds the valid Unicode range of U+10FFFF."_el);
        }
        return static_cast<char32_t>(value);
    } catch (const el::ParseError &) {
        throwError("Failed to parse a UCD code point."_el, std::current_exception());
    }
}

auto UcdReader::parse(const el::ItemCount minimumFields, const bool exactFields) -> el::List<UcdRecord> {
    static const auto cWhitespace = el::CharSet{U' ', U'\t', U'\r', U'\n'};
    // separators used in the UCD file
    static const auto fieldSeparator = el::CharSet{U';'};
    static const auto commentStart = el::CharSet{U'#'};
    static const auto codeRangeSeparator = ".."_el;
    auto result = el::List<UcdRecord>{};
    // read the file line by line
    auto lines = el::StringSplitter{_text, U'\n'};
    while (!lines.isAtEnd()) {
        // The split index is also the line number.
        _lineIndex = el::LineIndex::fromSizeTOrThrow(lines.splitIndex().toSizeT());
        auto line = lines.next().trimmed(cWhitespace);
        // Remove leading comment marker
        if (const auto parts = line.splitAtFirst(commentStart)) {
            line = parts->first.trimmed(cWhitespace); // Only keep text in front of a comment.
        }
        if (line.isEmpty()) {
            continue; // Skip lines without content.
        }
        // split the line into fields
        auto fields = el::StringList{};
        auto fieldSplitter = el::StringSplitter{line, fieldSeparator};
        while (!fieldSplitter.isAtEnd()) {
            fields.append(fieldSplitter.next().trimmed(cWhitespace));
        }
        const auto fieldCount = fields.count();
        if (fieldCount < minimumFields || (exactFields && fieldCount != minimumFields)) {
            throwError(
                el::StringFormat{"Malformed UCD record: expected {} fields, got {}."_el}.build(
                    minimumFields, fieldCount));
        }
        // interpret the code or code range
        const auto codeField = fields.get(el::ItemIndex{0}).trimmed(cWhitespace);
        char32_t first{};
        char32_t last{};
        if (const auto parts = codeField.splitAtFirst(codeRangeSeparator)) {
            const auto &[firstField, lastField] = *parts;
            first = codePoint(firstField.trimmed(cWhitespace));
            last = codePoint(lastField.trimmed(cWhitespace));
        } else {
            first = codePoint(codeField);
            last = first;
        }
        if (last < first) {
            throwError("Malformed or reversed UCD code-point range."_el);
        }
        result.append(UcdRecord{first, last, fields, _lineIndex});
    }
    return result;
}

void UcdReader::throwError(el::String description, const std::exception_ptr &cause) const {
    throw el::ApplicationError{"Failed to Parse Records from UCD File."_el, std::move(description), cause}
        .withSourcePath(_fileName)
        .withCodeLocation(el::CodeLocation{_lineIndex});
}

}
