// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OversizedNestedTypeRule.hpp"

#include "../SourceIndex.hpp"

#include <erbsland/RegEx.hpp>

#include <algorithm>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto OversizedNestedTypeRule::create() -> RulePtr {
    return std::make_unique<OversizedNestedTypeRule>();
}

auto OversizedNestedTypeRule::identifier() const -> el::String {
    return "oversized_nested_type"_el;
}

auto OversizedNestedTypeRule::title() const -> el::String {
    return "Nested type size"_el;
}

auto OversizedNestedTypeRule::description() const -> el::String {
    return "Detect nested types that should be extracted into their own units."_el;
}

auto OversizedNestedTypeRule::severity() const -> Severity {
    return Severity::High;
}

void OversizedNestedTypeRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Class && block.kind != SourceBlock::Kind::Struct) {
            continue;
        }
        const auto *parent = source.parent(block);
        auto nested = parent != nullptr &&
            (parent->kind == SourceBlock::Kind::Class || parent->kind == SourceBlock::Kind::Struct);
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        static const auto cQualified =
            el::RegEx::compile(R"(\b(?:class|struct)\s+[A-Z][A-Za-z0-9_]*(?:\s*<[^;{}]+>)?\s*::\s*[A-Za-z_]\w*)"_el);
        if (!nested && source.namespaceScope(source.typeStart(block)) && cQualified->findFirst(header) &&
            !header.contains("std::formatter"_el) && !header.contains("std::hash"_el)) {
            nested = true;
        }
        if (!nested) {
            continue;
        }
        const auto start = source.typeStart(block);
        auto codeLines = el::LineCount{};
        for (
            auto lineIndex = source.lineIndex(start); lineIndex <= source.lineIndex(block.closingBrace());
            ++lineIndex) {
            if (!source
                    .slice(
                        el::ByteRange{
                            std::max(start, source.lineStart(lineIndex)),
                            std::min(block.braces.endIndex(), source.lineEnd(lineIndex))},
                        true)
                    .trimmed()
                    .isEmpty()) {
                ++codeLines;
            }
        }
        if (codeLines > el::LineCount{10}) {
            auto end = block.braces.endIndex();
            const auto tail = source.slice(el::ByteRange{end, source.statementEnd(end)}, true).trimmed();
            if (tail.startsWith(";"_el)) {
                ++end;
            }
            candidates.append(el::ByteRange{start, end});
        }
    }
}

}
