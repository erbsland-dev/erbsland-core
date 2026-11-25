// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleNamespacesRule.hpp"

#include <erbsland/RegEx.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto MultipleNamespacesRule::create() -> RulePtr {
    return std::make_unique<MultipleNamespacesRule>();
}

auto MultipleNamespacesRule::identifier() const -> el::String {
    return "multiple_namespaces"_el;
}

auto MultipleNamespacesRule::title() const -> el::String {
    return "Multiple namespaces per unit"_el;
}

auto MultipleNamespacesRule::description() const -> el::String {
    return "Detect unrelated namespace declarations sharing one source unit."_el;
}

auto MultipleNamespacesRule::severity() const -> Severity {
    return Severity::High;
}

auto MultipleNamespacesRule::isStandardExtensionNamespace(const SourceIndex &source, const SourceBlock &block) -> bool {
    if (block.name != "std"_el) {
        return false;
    }
    static const auto cSpecialization = el::RegEx::compile(
        R"((?s)^template\s*<[^;{}]*>\s*(?:class|struct)\s+(?:hash|formatter)\s*<[^;{}]+>[^;{}]*$)"_el);
    auto position = block.openingBrace().incremented();
    auto foundSpecialization = false;
    for (const auto &child : source.blocks()) {
        if (source.parent(child) != &block) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{position, child.openingBrace()}, true).trimmed();
        if (!cSpecialization->fullMatch(header)) {
            return false;
        }
        position = child.braces.endIndex();
        // Consume exactly the specialization's terminating semicolon, never unrelated declarations.
        const auto tail = source.slice(el::ByteRange{position, block.closingBrace()}, true);
        const auto semicolon = tail.find(";"_el);
        if (semicolon.isNoIndex() || !tail.slice(el::ByteRange{el::ByteIndex::zero(), semicolon}).trimmed().isEmpty()) {
            return false;
        }
        position += semicolon.distanceFromZero() + el::ByteCount{1};
        foundSpecialization = true;
    }
    return foundSpecialization && source.slice(el::ByteRange{position, block.closingBrace()}, true).trimmed().isEmpty();
}

void MultipleNamespacesRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    auto names = el::StringSet{};
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Namespace || block.name.isEmpty() || block.parent.isValid() ||
            source.insideOsConditional(block.openingBrace()) || isStandardExtensionNamespace(source, block)) {
            continue;
        }
        if (!names.tryInsert(block.name) || names.count() == el::ItemCount{1}) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto keyword = header.find("namespace"_el);
        if (!keyword.isNoIndex()) {
            candidates.append(
                el::ByteRange{block.headerStart + keyword.distanceFromZero(), block.openingBrace().incremented()});
        }
    }
}

}
