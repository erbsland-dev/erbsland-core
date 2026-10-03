// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ImplementationInWrongUnitRule.hpp"

#include "../SourceIndex.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto ImplementationInWrongUnitRule::create() -> RulePtr {
    return std::make_unique<ImplementationInWrongUnitRule>();
}

auto ImplementationInWrongUnitRule::identifier() const -> el::String {
    return "implementation_in_wrong_unit"_el;
}

auto ImplementationInWrongUnitRule::title() const -> el::String {
    return "Implementation unit placement"_el;
}

auto ImplementationInWrongUnitRule::description() const -> el::String {
    return "Detect definitions whose owning type belongs in another implementation unit."_el;
}

auto ImplementationInWrongUnitRule::severity() const -> Severity {
    return Severity::High;
}

void ImplementationInWrongUnitRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().suffix() != ".cpp"_el) {
        return;
    }
    static const auto cOwner =
        el::RegEx::compile(R"(\b([A-Z][A-Za-z0-9_]*)\s*::\s*(?:~?[A-Za-z_]\w*|operator\S*)\s*\()"_el);
    static const auto cOuter = el::RegEx::compile(R"(([A-Z][A-Za-z0-9_]*)\s*::\s*$)"_el);
    const auto unit = source.path().stem();
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto start = source.lineStart(lineIndex);
        const auto line = source.slice(el::ByteRange{start, source.lineEnd(lineIndex)}, true);
        if (!line.contains("::"_el) || !line.contains("("_el)) {
            continue;
        }
        if (const auto match = cOwner->findFirst(line)) {
            // RegEx positions are raw byte offsets for this UTF-8 input.
            const auto position = start + el::ByteCount::fromSizeT(match->begin());
            const auto prefix = source.slice(el::ByteRange{start, position}, true).trimmed();
            if (prefix.contains("("_el) || prefix.contains(")"_el) || prefix.contains("="_el) ||
                prefix.startsWith("return"_el) || prefix.startsWith("co_return"_el)) {
                continue;
            }
            if (!source.namespaceScope(position)) {
                continue;
            }
            auto owner = match->content(1);
            if (const auto outer = cOuter->findFirst(prefix)) {
                owner = outer->content(1);
            }
            if (unit != owner && !unit.startsWith(el::join(owner, "_"_el))) {
                candidates.append(el::ByteRange{position, source.statementEnd(position)});
            }
        }
    }
}

}
