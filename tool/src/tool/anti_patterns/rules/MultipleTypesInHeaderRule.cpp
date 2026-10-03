// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleTypesInHeaderRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/StringJoins.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto MultipleTypesInHeaderRule::create() -> RulePtr {
    return std::make_unique<MultipleTypesInHeaderRule>();
}

auto MultipleTypesInHeaderRule::identifier() const -> el::String {
    return "multiple_types_in_header"_el;
}

auto MultipleTypesInHeaderRule::title() const -> el::String {
    return "Multiple primary types per header"_el;
}

auto MultipleTypesInHeaderRule::description() const -> el::String {
    return "Detect headers containing more than one primary type."_el;
}

auto MultipleTypesInHeaderRule::severity() const -> Severity {
    return Severity::Medium;
}

void MultipleTypesInHeaderRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().suffix() != ".hpp"_el) {
        return;
    }
    auto types = el::List<const SourceBlock *>{};
    for (const auto &block : source.blocks()) {
        if ((block.kind == SourceBlock::Kind::Class || block.kind == SourceBlock::Kind::Struct ||
                block.kind == SourceBlock::Kind::EnumClass) &&
            impl::namespaceType(source, block)) {
            types.append(&block);
        }
    }
    if (types.count() <= el::ItemCount{1}) {
        return;
    }
    const auto *primary = types.first();
    for (const auto *block : types) {
        if (block->name == source.path().stem()) {
            primary = block;
            break;
        }
    }
    const auto internal = source.path().elements().contains("impl"_el);
    for (const auto *block : types) {
        if (block == primary) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{block->headerStart, block->openingBrace()}, true);
        if (impl::standardExtension(source, block->headerStart, header)) {
            continue;
        }
        const auto primaryHeader = source.slice(el::ByteRange{primary->headerStart, primary->openingBrace()}, true);
        const auto primaryName = primaryHeader.find(primary->name);
        const auto primaryTail = primaryName.isNoIndex()
            ? el::String{}
            : primaryHeader.slice(el::StringSide::Back, primaryName + primary->name.length()).trimmed();
        if (block->name == primary->name && source.path().stem() == primary->name && !primaryTail.startsWith("<"_el) &&
            header.contains("template"_el) && header.contains(el::join(block->name, "<"_el))) {
            continue;
        }
        if (internal && (block->kind == SourceBlock::Kind::Class || block->kind == SourceBlock::Kind::Struct) &&
            source.lineIndex(block->headerStart).absoluteDistanceTo(source.lineIndex(block->closingBrace())) <
                el::LineCount{11} &&
            !source.slice(el::ByteRange{block->openingBrace().incremented(), block->closingBrace()}, true)
                .contains("("_el)) {
            continue;
        }
        candidates.append(el::ByteRange{source.typeStart(*block), block->openingBrace().incremented()});
    }
}

}
