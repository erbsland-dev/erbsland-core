// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiTestStatus.hpp"

#include <erbsland/CharSet.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/UnicodeCategory.hpp>
#include <tool/cleanup/CleanupText.hpp>

namespace tool::doc::impl::test_status {

using namespace el::text::literals;

auto normalize(const el::String &text) -> el::String {
    static const auto reStatusMarker = el::RegEx::compile(R"re((?x)
        ^ (?<indent>\s*) /// \s* @             # Documentation marker and retained indentation
        (?<name>tested|notest|needtest)        # Supported test-status name
        (?<value>.*) $                         # Remaining marker payload
    )re"_el);
    static const auto reCommentContinuation = el::RegEx::compile(R"re(^\s*///\s?(?<text>.*)$)re"_el);
    static const auto reTypeDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?: template\b | requires\b          # Template or constraint preamble
             | class\b | struct\b | using\b )  # Type declarations
    )re"_el);
    static const auto reFunctionDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?:(?: \[\[[^\]]+\]\] | inline | constexpr ) \s+)*    # Attributes and qualifiers
        (?:auto|void) \b                                        # Supported function return spelling
    )re"_el);
    static const auto reBracedValue = el::RegEx::compile(R"re(^\{(.*?)\}.*$)re"_el);
    static const auto suiteSeparators =
        el::CharSet::from(el::UnicodeCategory::SpaceSeparator) | el::CharSet{U',', U'\t'};

    const auto lines = cleanup::text::lines(text);
    auto output = el::StringList{};
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count()); ++index) {
        const auto found = reStatusMarker->match(lines.getRef(index));
        if (!found) {
            output.append(lines.getRef(index));
            continue;
        }
        auto value = found->content("value"_el).trimmed();
        while (!value.contains("}"_el) && (index + el::ItemCount{1}).isWithin(lines.count())) {
            const auto next = reCommentContinuation->match(lines.getRef(index + el::ItemCount{1}));
            if (!next) {
                break;
            }
            ++index;
            value = el::join(value, " "_el, next->content("text"_el).trimmed());
        }
        auto declaration = el::String{};
        for (auto next = index + el::ItemCount{1}; next.isWithin(lines.count()); ++next) {
            declaration = lines.getRef(next).trimmed();
            if (!declaration.isEmpty() && !declaration.startsWith("///"_el)) {
                break;
            }
        }
        const auto type = reTypeDeclaration->match(declaration);
        const auto function = reFunctionDeclaration->match(declaration);
        if (!type && !(found->content("indent"_el).isEmpty() && function)) {
            continue;
        }
        value = reBracedValue->replaceAll(value, "{1}"_el);
        if (found->content("name"_el) == "tested"_el) {
            value = value.replacedAll("`"_el, ""_el);
            auto suites = el::StringList{};
            for (const auto &suite : el::StringList::fromSplit(value, suiteSeparators)) {
                auto name = suite;
                if (const auto separator = suite.splitAtFirst("::"_el)) {
                    name = separator->first;
                }
                if (!suites.contains(name)) {
                    suites.append(name);
                }
            }
            value = suites.join(" "_el);
        }
        output.append(
            el::join(
                found->content("indent"_el), "/// @"_el, found->content("name"_el), "{"_el, value.trimmed(), "}"_el));
    }
    auto result = output.join("\n"_el);
    if (text.endsWith("\n"_el)) {
        result = el::join(result, "\n"_el);
    }
    return result;
}

auto validate(const el::String &text) -> el::StringList {
    static const auto reStatusMarker = el::RegEx::compile(R"re((?x)
        ^ \s* /// \s* @                      # Documentation marker
        (?<name>tested|notest|needtest)      # Supported test-status name
        (?<value>.*) $                       # Remaining marker payload
    )re"_el);
    static const auto reBracedValue = el::RegEx::compile(R"re(^\{.+\}$)re"_el);
    static const auto reTestedSuites = el::RegEx::compile(R"re((?x)
        ^ \{ [A-Za-z_]\w*Test      # First suite class
        (?: [ ] [A-Za-z_]\w*Test )*      # Further suites separated by one literal space
        \} $                           # End of marker
    )re"_el);
    static const auto reTypeDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?: template\b | requires\b   # Template or constraint preamble
             | class\b | struct\b | using\b ) # Type declarations
    )re"_el);
    static const auto reFunctionDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?:(?: \[\[[^\]]+\]\] | inline | constexpr ) \s+)* # Attributes and qualifiers
        (?:auto|void) \b # Supported function return spelling
    )re"_el);

    auto issues = el::StringList{};
    const auto lines = cleanup::text::lines(text);
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count()); ++index) {
        const auto found = reStatusMarker->match(lines.getRef(index));
        if (!found) {
            continue;
        }
        auto reason = el::String{};
        const auto name = found->content("name"_el);
        const auto value = found->content("value"_el).trimmed();
        if (!reBracedValue->match(value)) {
            reason = "marker requires a single-line nonempty braced value."_el;
        } else if (name == "tested"_el && !reTestedSuites->match(value)) {
            reason = "@tested must contain space-separated suite names ending in Test."_el;
        }
        auto declaration = el::String{};
        for (auto next = index + el::ItemCount{1}; next.isWithin(lines.count()); ++next) {
            declaration = lines.getRef(next).trimmed();
            if (!declaration.isEmpty() && !declaration.startsWith("///"_el)) {
                break;
            }
        }
        const auto type = reTypeDeclaration->match(declaration);
        const auto function = reFunctionDeclaration->match(declaration);
        if (!type && !(lines.getRef(index).startsWith("///"_el) && function)) {
            reason = "test-status markers are only allowed on types and free functions."_el;
        }
        if (!reason.isEmpty()) {
            issues.append(el::join(el::String::fromInteger(index.incremented().toRawValue()), ": "_el, reason));
        }
    }
    return issues;
}

}
