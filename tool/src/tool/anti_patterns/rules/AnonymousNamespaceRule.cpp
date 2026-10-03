// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnonymousNamespaceRule.hpp"

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto AnonymousNamespaceRule::create() -> RulePtr {
    return std::make_unique<AnonymousNamespaceRule>();
}

auto AnonymousNamespaceRule::identifier() const -> el::String {
    return "anonymous_namespace"_el;
}

auto AnonymousNamespaceRule::title() const -> el::String {
    return "Anonymous namespaces"_el;
}

auto AnonymousNamespaceRule::description() const -> el::String {
    return "Detect declarations hidden in anonymous namespaces."_el;
}

auto AnonymousNamespaceRule::severity() const -> Severity {
    return Severity::High;
}

void AnonymousNamespaceRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Namespace || !block.name.isEmpty()) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto keyword = header.find("namespace"_el);
        if (!keyword.isNoIndex()) {
            const auto start = block.headerStart + keyword.distanceFromZero();
            candidates.append(el::ByteRange{start, source.statementEnd(start)});
        }
    }
}

}
