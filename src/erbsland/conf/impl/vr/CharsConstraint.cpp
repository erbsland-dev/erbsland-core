// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharsConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../char/NamedChars.hpp"

#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/u8/U8StringConstIterator.hpp"
#include "../../ConfError.hpp"

#include <algorithm>
#include <unordered_set>

namespace erbsland::conf::impl {

using namespace text::literals;

void CharsConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    std::size_t index = 0;
    for (const auto character : value) {
        const auto inRanges = _charSet.contains(character);
        const auto isInvalid = isNegated() ? inRanges : !inRanges;
        if (isInvalid) {
            if (context.rule != nullptr && context.rule->isSecret()) {
                throwValidationError(
                    text::StringFormat{"The text contains a forbidden character at position {} in a secret value"_el}
                        .build(index));
            }
            throwValidationError(
                text::StringFormat{"The text contains a forbidden character at position {}: \"{:/display}\""_el}.build(
                    index, text::String::fromCharacter(character)));
        }
        ++index;
    }
}

auto CharsConstraint::parseTextRanges(const text::StringList &texts) -> text::CharSet {
    text::CharSet result;

    for (const auto &text : texts) {
        std::vector<text::Char> cps;
        auto reader = text::StringCharReader{text};
        for (auto character = reader.read(); character != text::Char::endOfData(); character = reader.read()) {
            cps.emplace_back(character);
        }

        if (cps.size() >= 2 && cps.front() == U'(' && cps.back() == U')') {
            parseParenRange(cps, text, result);
        } else if (cps.size() >= 2 && cps.front() == U'[' && cps.back() == U']') {
            parseBracketList(cps, result);
        } else if (!tryAppendNamedRange(text, namedRanges(), result)) {
            throwValidationError(text::StringFormat{"Unknown named character range: \"{:/display}\""_el}.build(text));
        }
    }

    return result;
}

auto CharsConstraint::namedRanges() -> const std::vector<NamedRange> & {
    static const auto namedRanges = std::vector<NamedRange>{
        {"letters"_el,
            text::CharSet::fromRange(nc::lowercaseA, nc::lowercaseZ) |
                text::CharSet::fromRange(nc::uppercaseA, nc::uppercaseZ)},
        {"digits"_el, text::CharSet::fromRange(nc::digit0, nc::digit9)},
        {"control"_el,
            text::CharSet::fromRange(text::Char{0x0000U}, text::Char{0x001FU}) |
                text::CharSet::fromRange(text::Char{0x007FU}, text::Char{0x00A0U})},
        {"linebreak"_el, text::CharSet{nc::newLine, nc::carriageReturn}},
        {"spacing"_el, text::CharSet{nc::tab, nc::space}}};
    return namedRanges;
}

void CharsConstraint::parseParenRange(
    const std::vector<text::Char> &cps, const text::String &rawText, text::CharSet &out) {
    // Must be exactly 5 code points: '(', start, '-', end, ')'
    if (cps.size() != 5 || !(cps[2] == U'-')) {
        throwValidationError(text::StringFormat{"Invalid character range syntax: \"{:/display}\""_el}.build(rawText));
    }
    const text::Char start = cps[1];
    const text::Char end = cps[3];
    if (!(start < end)) {
        throwValidationError(
            text::StringFormat{"Invalid character range: start (U+{:04X}) must be lower than end (U+{:04X})"_el}.build(
                static_cast<unsigned>(start.toRawValue()), static_cast<unsigned>(end.toRawValue())));
    }
    out.add(text::CharRange{start, end});
}

void CharsConstraint::parseBracketList(const std::vector<text::Char> &cps, text::CharSet &out) {
    std::vector<text::Char> seen;
    for (std::size_t i = 1; i + 1 < cps.size(); ++i) {
        const text::Char c = cps[i];
        if (std::ranges::find(seen, c) != seen.end()) {
            throwValidationError(
                text::StringFormat{"The character list contains a duplicate character: '{:/display}'"_el}.build(
                    text::String::fromCharacter(c)));
        }
        seen.push_back(c);
        out.add(c);
    }
}

auto CharsConstraint::tryAppendNamedRange(
    const text::String &text, const std::vector<NamedRange> &named, text::CharSet &out) -> bool {
    for (const auto &[name, ranges] : named) {
        if (name == text) {
            out.add(ranges);
            return true;
        }
    }
    return false;
}

auto handleCharsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (rule->type() == vr::RuleType::Text) {
        const auto textValues = node->asList<text::String>();
        if (textValues.empty()) {
            throwValidationError(
                text::StringFormat{"The '{}' constraint must specify a single text value or a list of texts"_el}.build(
                    node->name().asText()));
        }
        return std::make_shared<CharsConstraint>(text::StringList{textValues});
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            node->name().asText(), rule->type().toText()));
}

}
