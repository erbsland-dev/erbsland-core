// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiDocumentationOnMemberImplementationRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/Set.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto ApiDocumentationOnMemberImplementationRule::create() -> RulePtr {
    return std::make_unique<ApiDocumentationOnMemberImplementationRule>();
}

auto ApiDocumentationOnMemberImplementationRule::identifier() const -> el::String {
    return "api_documentation_on_member_implementation"_el;
}

auto ApiDocumentationOnMemberImplementationRule::title() const -> el::String {
    return "Documentation on member implementations"_el;
}

auto ApiDocumentationOnMemberImplementationRule::description() const -> el::String {
    return "Detect API comments on out-of-line member definitions instead of their declarations."_el;
}

auto ApiDocumentationOnMemberImplementationRule::severity() const -> Severity {
    return Severity::Medium;
}

void ApiDocumentationOnMemberImplementationRule::scan(
    const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().suffix() != ".hpp"_el && source.path().suffix() != ".tpp"_el) {
        return;
    }
    auto ends = el::Set<el::ByteIndex>{};
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto lineStart = source.lineStart(lineIndex);
        const auto line = source.slice(el::ByteRange{lineStart, source.lineEnd(lineIndex)}, true);
        const auto compact = line.trimmed();
        if (!compact.contains("::"_el) || !compact.contains("("_el)) {
            continue;
        }
        const auto start = lineStart + line.find(compact).distanceFromZero();
        if (!impl::outOfClassMember(source, start) || !impl::hasApiDocumentation(source, start)) {
            continue;
        }
        const auto end = source.statementEnd(start);
        if (!ends.contains(end)) {
            ends.insert(end);
            candidates.append(el::ByteRange{start, end});
        }
    }
}

}
