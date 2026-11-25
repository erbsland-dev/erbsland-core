// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StaticGlobalObjectRule.hpp"

#include "../SourceIndex.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto StaticGlobalObjectRule::create() -> RulePtr {
    return std::make_unique<StaticGlobalObjectRule>();
}

auto StaticGlobalObjectRule::identifier() const -> el::String {
    return "static_global_object"_el;
}

auto StaticGlobalObjectRule::title() const -> el::String {
    return "Static global objects"_el;
}

auto StaticGlobalObjectRule::description() const -> el::String {
    return "Detect namespace-scope objects with static initialization or destruction."_el;
}

auto StaticGlobalObjectRule::severity() const -> Severity {
    return Severity::High;
}

void StaticGlobalObjectRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    static const auto cObject = el::RegEx::compile(
        R"(^(?:(?:inline|static|const|volatile)\s+)*((?:[A-Z]\w*|(?:[A-Za-z_]\w*::)+[A-Za-z_]\w*)(?:\s*<[^;{}=()]+>)?)\s+[A-Za-z_]\w*\s*(?:\{|=))"_el);
    static const auto cArray = el::RegEx::compile(
        R"(^(?:(?:inline|static|const|volatile)\s+)*[A-Za-z_][A-Za-z0-9_:<> ,]*\s+[A-Za-z_]\w*\s*\[[^\]]*\]\s*(?:\{|=))"_el);
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto start = source.lineStart(lineIndex);
        const auto line = source.slice(el::ByteRange{start, source.lineEnd(lineIndex)}, true);
        const auto trimmed = line.trimmed();
        if (trimmed.startsWith("constexpr "_el) || trimmed.startsWith("constinit "_el) ||
            trimmed.startsWith("extern "_el) || trimmed.startsWith("thread_local "_el)) {
            continue;
        }
        const auto match = cObject->findFirst(trimmed) ? cObject->findFirst(trimmed) : cArray->findFirst(trimmed);
        if (!match) {
            continue;
        }
        const auto offset = start + line.find(trimmed).distanceFromZero();
        if (source.namespaceScope(offset) && !trimmed.contains("*"_el) && !trimmed.contains("&"_el) &&
            !source.hasUnclosedDelimiterBefore(offset, "("_el, ")"_el) &&
            !source.hasUnclosedDelimiterBefore(offset, "<"_el, ">"_el)) {
            candidates.append(el::ByteRange{offset, source.statementEnd(offset)});
        }
    }
}

}
