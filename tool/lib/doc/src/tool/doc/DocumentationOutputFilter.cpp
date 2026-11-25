// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentationOutputFilter.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::doc {

using namespace el::text::literals;

DocumentationOutputFilter::DocumentationOutputFilter(el::Path root, bool showSuppressed) :
    _generatedPrefix{el::join((root / "_doxygen_input"_el).toString(), "/"_el)},
    _documentationPrefix{el::join((root / "doc"_el).toString(), "/"_el)},
    _showSuppressed{showSuppressed} {
    const auto extendedRoot = root.toWindows().replacedAll("\\"_el, "/"_el);
    if (!extendedRoot.isEmpty()) {
        _extendedGeneratedPrefix = el::join(extendedRoot, "/_doxygen_input/"_el);
        _extendedDocumentationPrefix = el::join(extendedRoot, "/doc/"_el);
    }
}

void DocumentationOutputFilter::addRule(
    const el::String &sourceGlob, el::String contains, const el::String &regex, el::String reason) {
    auto expression = el::re::RegExPtr{};
    if (!regex.isEmpty()) {
        expression = el::re::RegEx::compile(regex);
    }
    _rules.append(
        DoxygenWarningSuppression{
            el::PathGlobPattern::compile(sourceGlob), std::move(contains), std::move(expression), std::move(reason)});
}

auto DocumentationOutputFilter::shortenPaths(const el::String &line) const -> el::String {
    static const auto reGeneratedPath = el::RegEx::compile(R"((^|\s)(?:[A-Za-z]:)?/.*?/_doxygen_input/)"_el);
    static const auto reDocPath = el::RegEx::compile(R"re((?x)
        (^|\s)                 # Retain the preceding separator
        (?:[A-Za-z]:)? / .*? / # Optional drive and containing directories
        erbsland-core (?: - [A-Za-z0-9_]+ )* /doc/ # Core or extension checkout
    )re"_el);
    auto result = line;
    if (!_extendedGeneratedPrefix.isEmpty()) {
        // Replace the longest spellings first, avoiding a leftover //?/ or //?/UNC/ prefix.
        result = result.replacedAll(_extendedGeneratedPrefix, ""_el);
        result = result.replacedAll(_extendedDocumentationPrefix, ""_el);
    }
    result = result.replacedAll(_generatedPrefix, ""_el);
    result = result.replacedAll(_documentationPrefix, ""_el);
    const auto prefix = [](const el::re::MatchPtr &match) -> el::String { return match->content(1); };
    result = reGeneratedPath->replaceAll(result, prefix);
    return reDocPath->replaceAll(result, prefix);
}

auto DocumentationOutputFilter::filterLine(const el::String &line) -> std::optional<el::String> {
    static const auto reDoxygen = el::RegEx::compile(R"((?i)^(.+?):\d+:\s+warning:\s+(.*)$)"_el);
    if (line.contains("ERROR:"_el) || line.contains("error:"_el) || line.contains("WARNING:"_el)) {
        return shortenPaths(line);
    }
    const auto match = reDoxygen->match(line);
    if (match == nullptr) {
        return {};
    }
    const auto path = shortenPaths(match->content(1));
    const auto message = match->content(2);
    for (auto index = el::ItemIndex{}; index.isWithin(_rules.count()); ++index) {
        auto rule = _rules.getRefOrThrow(index);
        if (!rule.source->fullMatch(path) || !message.contains(rule.messageContains) ||
            (rule.messageRegex != nullptr && rule.messageRegex->findFirst(message) == nullptr)) {
            continue;
        }
        if (_showSuppressed) {
            return shortenPaths(line);
        }
        if (rule.count.isZero()) {
            _suppressed.append(index);
        }
        ++rule.count;
        _rules.set(index, rule);
        return {};
    }
    return shortenPaths(line);
}

auto DocumentationOutputFilter::summary() const -> el::StringList {
    auto result = el::StringList{};
    for (const auto index : _suppressed) {
        const auto &rule = _rules.getRefOrThrow(index);
        result.append(
            el::StringFormat{"Suppressed {} Doxygen warning(s): {}"_el}.build(rule.count.toRawValue(), rule.reason));
    }
    return result;
}

}
