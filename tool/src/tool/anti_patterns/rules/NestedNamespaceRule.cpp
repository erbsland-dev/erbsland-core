// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NestedNamespaceRule.hpp"

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto NestedNamespaceRule::create() -> RulePtr {
    return std::make_unique<NestedNamespaceRule>();
}

auto NestedNamespaceRule::identifier() const -> el::String {
    return "nested_namespace"_el;
}

auto NestedNamespaceRule::title() const -> el::String {
    return "Nested namespace declarations"_el;
}

auto NestedNamespaceRule::description() const -> el::String {
    return "Detect nested namespace blocks that can use a qualified declaration."_el;
}

auto NestedNamespaceRule::severity() const -> Severity {
    return Severity::Low;
}

void NestedNamespaceRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Namespace) {
            continue;
        }
        const auto *parent = source.parent(block);
        if (!block.name.isEmpty() && parent != nullptr && parent->kind == SourceBlock::Kind::Namespace &&
            !parent->name.isEmpty()) {
            const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
            const auto keyword = header.find("namespace"_el);
            if (!keyword.isNoIndex()) {
                candidates.append(
                    el::ByteRange{block.headerStart + keyword.distanceFromZero(), block.openingBrace().incremented()});
            }
        }
        const auto closingLine = source.lineIndex(block.closingBrace());
        for (const auto index : source.comments()) {
            const auto &comment = source.tokens().getRef(index);
            const auto start = comment.position();
            if (start > block.closingBrace() && source.lineIndex(start) == closingLine &&
                source.slice(el::ByteRange{block.braces.endIndex(), start}).trimmed().isEmpty()) {
                candidates.append(el::ByteRange{start, comment.range().endIndex()});
                break;
            }
        }
    }
}

}
