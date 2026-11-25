// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamespaceInWrongUnitRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto NamespaceInWrongUnitRule::create() -> RulePtr {
    return std::make_unique<NamespaceInWrongUnitRule>();
}

auto NamespaceInWrongUnitRule::identifier() const -> el::String {
    return "namespace_in_wrong_unit"_el;
}

auto NamespaceInWrongUnitRule::title() const -> el::String {
    return "Namespace directory placement"_el;
}

auto NamespaceInWrongUnitRule::description() const -> el::String {
    return "Detect namespaces whose names do not match their source directories."_el;
}

auto NamespaceInWrongUnitRule::severity() const -> Severity {
    return Severity::High;
}

void NamespaceInWrongUnitRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Namespace || block.name.isEmpty()) {
            continue;
        }
        const auto name = impl::namespaceName(source, block, true);
        if (!name.startsWith("erbsland"_el) || impl::matchesSourceDirectory(source, name)) {
            continue;
        }
        if (name == "erbsland"_el && source.path().toString().startsWith("src/erbsland/"_el)) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto keyword = header.find("namespace"_el);
        const auto start = keyword.isNoIndex() ? block.headerStart : block.headerStart + keyword.distanceFromZero();
        candidates.append(el::ByteRange{start, block.openingBrace().incremented()});
    }
}

}
