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

using namespace text;
using namespace text::literals;

void CharsConstraint::validateText(const ValidationContext &context, const String &value) const {
    std::size_t index = 0;
    for (const auto character : value) {
        const auto inRanges = _charSet.contains(character);
        const auto isInvalid = isNegated() ? inRanges : !inRanges;
        if (isInvalid) {
            if (context.rule != nullptr && context.rule->isSecret()) {
                throwValidationError(
                    StringFormat{"The text contains a forbidden character at position {} in a secret value"_el}.build(
                        index));
            }
            throwValidationError(
                StringFormat{"The text contains a forbidden character at position {}: \"{:/display}\""_el}.build(
                    index, String::fromCharacter(character)));
        }
        ++index;
    }
}

auto CharsConstraint::parseTextRanges(const StringList &texts) -> CharSet {
    CharSet result;

    for (const auto &text : texts) {
        std::vector<Char> cps;
        auto reader = StringCharReader{text};
        for (auto character = reader.read(); character != Char::endOfData(); character = reader.read()) {
            cps.emplace_back(character);
        }

        if (cps.size() >= 2 && cps.front() == U'(' && cps.back() == U')') {
            parseParenRange(cps, text, result);
        } else if (cps.size() >= 2 && cps.front() == U'[' && cps.back() == U']') {
            parseBracketList(cps, result);
        } else if (!tryAppendNamedRange(text, namedRanges(), result)) {
            throwValidationError(StringFormat{"Unknown named character range: \"{:/display}\""_el}.build(text));
        }
    }

    return result;
}

auto CharsConstraint::namedRanges() -> const std::vector<NamedRange> & {
    static const auto namedRanges = std::vector<NamedRange>{
        {"letters"_el,
            CharSet::fromRange(nc::lowercaseA, nc::lowercaseZ) | CharSet::fromRange(nc::uppercaseA, nc::uppercaseZ)},
        {"digits"_el, CharSet::fromRange(nc::digit0, nc::digit9)},
        {"control"_el,
            CharSet::fromRange(Char{0x0000U}, Char{0x001FU}) | CharSet::fromRange(Char{0x007FU}, Char{0x00A0U})},
        {"linebreak"_el, CharSet{nc::newLine, nc::carriageReturn}},
        {"spacing"_el, CharSet{nc::tab, nc::space}}};
    return namedRanges;
}

void CharsConstraint::parseParenRange(const std::vector<Char> &cps, const String &rawText, CharSet &out) {
    // Must be exactly 5 code points: '(', start, '-', end, ')'
    if (cps.size() != 5 || !(cps[2] == U'-')) {
        throwValidationError(StringFormat{"Invalid character range syntax: \"{:/display}\""_el}.build(rawText));
    }
    const Char start = cps[1];
    const Char end = cps[3];
    if (!(start < end)) {
        throwValidationError(
            StringFormat{"Invalid character range: start (U+{:04X}) must be lower than end (U+{:04X})"_el}.build(
                static_cast<unsigned>(start.toRawValue()), static_cast<unsigned>(end.toRawValue())));
    }
    out.add(CharRange{start, end});
}

void CharsConstraint::parseBracketList(const std::vector<Char> &cps, CharSet &out) {
    std::vector<Char> seen;
    for (std::size_t i = 1; i + 1 < cps.size(); ++i) {
        const Char c = cps[i];
        if (std::ranges::find(seen, c) != seen.end()) {
            throwValidationError(
                StringFormat{"The character list contains a duplicate character: '{:/display}'"_el}.build(
                    String::fromCharacter(c)));
        }
        seen.push_back(c);
        out.add(c);
    }
}

auto CharsConstraint::tryAppendNamedRange(const String &text, const std::vector<NamedRange> &named, CharSet &out)
    -> bool {
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
        const auto textValues = node->asStringList();
        if (textValues.isEmpty()) {
            throwValidationError(
                StringFormat{"The '{}' constraint must specify a single text value or a list of texts"_el}.build(
                    node->name().asText()));
        }
        return std::make_shared<CharsConstraint>(textValues);
    }
    throwValidationError(
        StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            node->name().asText(), rule->type().toText()));
}

}
