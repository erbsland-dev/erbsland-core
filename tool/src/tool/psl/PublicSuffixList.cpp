// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicSuffixList.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/Map.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringSplitter.hpp>

namespace tool::psl {

using namespace el::text::literals;

PublicSuffixList::PublicSuffixList(
    const el::String &text, const el::String &source, const idna::LabelProcessor &processor) :
    _source{source} {
    auto splitter = el::StringSplitter{text, U'\n'};
    auto lineIndex = el::LineIndex{};
    auto state = ParseState{};
    auto canonical = el::Map<el::String, unsigned>{};
    while (!splitter.isAtEnd()) {
        const auto line = splitter.next().trimmed();
        if (line.startsWith(cVersionLabel)) {
            parseVersion(line, lineIndex);
        } else if (line.startsWith("//"_el)) {
            parseSection(line, lineIndex, state);
        } else if (!line.isEmpty()) {
            if (state.section == 0U) {
                throwError("A rule appears outside a PSL section."_el, lineIndex);
            }
            const auto rule = parseRule(line, lineIndex, processor);
            if (canonical.contains(rule.text) && canonical.get(rule.text, 0U) != rule.token) {
                throwError("Rule families overlap after canonicalization."_el, lineIndex);
            }
            canonical.set(rule.text, rule.token);
        }
        ++lineIndex;
    }
    if (_version.isEmpty() || !state.seenIcann || !state.seenPrivate || state.section != 0U ||
        canonical.count() == el::ItemCount{}) {
        throwError("Missing version, sections, rules, or section terminator."_el, lineIndex);
    }
    for (const auto &[rule, token] : canonical) {
        _rules.append(Rule{rule, token});
    }
    _rules.sort();
}

void PublicSuffixList::parseVersion(const el::String &line, const el::LineIndex lineIndex) {
    if (!_version.isEmpty()) {
        throwError("Duplicate snapshot version."_el, lineIndex);
    }
    _version = line.slice(el::StringSide::Back, el::ByteIndex::end(cVersionLabel.length()));
    static const auto versionPattern = el::re::RegEx::compile(R"([A-Za-z0-9_.\-]+)"_el);
    if (!versionPattern->fullMatch(_version)) {
        throwError("Invalid snapshot version syntax."_el, lineIndex);
    }
}

void PublicSuffixList::parseSection(const el::String &line, const el::LineIndex lineIndex, ParseState &state) const {
    if (line == "// ===BEGIN ICANN DOMAINS==="_el) {
        if (state.section != 0U || state.seenIcann || state.seenPrivate) {
            throwError("Unexpected ICANN section start."_el, lineIndex);
        }
        state.seenIcann = true;
        state.section = 1U;
    } else if (line == "// ===BEGIN PRIVATE DOMAINS==="_el) {
        if (state.section != 0U || !state.seenIcann || state.seenPrivate) {
            throwError("Unexpected PRIVATE section start."_el, lineIndex);
        }
        state.seenPrivate = true;
        state.section = 2U;
    } else if (line == "// ===END ICANN DOMAINS==="_el || line == "// ===END PRIVATE DOMAINS==="_el) {
        auto expected = 2U;
        if (line == "// ===END ICANN DOMAINS==="_el) {
            expected = 1U;
        }
        if (state.section != expected) {
            throwError("Unexpected section end."_el, lineIndex);
        }
        state.section = 0U;
    }
}

auto PublicSuffixList::parseRule(
    const el::String &line, const el::LineIndex lineIndex, const idna::LabelProcessor &processor) const -> Rule {
    static constexpr auto cExceptionPrefix = "!"_el;
    static constexpr auto cWildcardPrefix = "*."_el;
    auto token = 0U;
    auto base = line;
    if (base.startsWith(cExceptionPrefix)) {
        token = 2U;
        base = base.slice(el::StringSide::Back, el::ByteIndex::end(cExceptionPrefix.length()));
    } else if (base.startsWith(cWildcardPrefix)) {
        token = 1U;
        base = base.slice(el::StringSide::Back, el::ByteIndex::end(cWildcardPrefix.length()));
    }
    static const auto whitespace = el::CharSet::from(el::AsciiCategory::Whitespace);
    if (base.isEmpty() || base.contains("!"_el) || base.contains("*"_el) || base.findFirstOf(whitespace).isValid()) {
        throwError("Malformed PSL rule syntax."_el, lineIndex);
    }
    el::String ascii;
    try {
        ascii = processor.toAscii(base);
    } catch (const el::RuntimeError &) {
        throw el::ApplicationError{
            "Invalid Public Suffix Rule"_el,
            "Unicode preparation or IDNA validation failed."_el,
            std::current_exception()}
            .withSourcePath(_source)
            .withCodeLocation(el::CodeLocation{lineIndex});
    }
    return Rule{reverseLabels(ascii), token};
}

auto PublicSuffixList::reverseLabels(const el::String &text) -> el::String {
    return el::StringList::fromSplit(text, el::CharSet{U'.'}, el::ItemCount::infinite(), true).reversed().join("."_el);
}

void PublicSuffixList::throwError(const el::String &message, const el::LineIndex line) const {
    throw el::ApplicationError{"Malformed Public Suffix List"_el, message}.withSourcePath(_source).withCodeLocation(
        el::CodeLocation{line});
}

}
