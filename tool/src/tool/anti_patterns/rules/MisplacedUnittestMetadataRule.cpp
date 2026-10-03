// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MisplacedUnittestMetadataRule.hpp"

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto MisplacedUnittestMetadataRule::create() -> RulePtr {
    return std::make_unique<MisplacedUnittestMetadataRule>();
}

auto MisplacedUnittestMetadataRule::identifier() const -> el::String {
    return "misplaced_unittest_metadata"_el;
}

auto MisplacedUnittestMetadataRule::title() const -> el::String {
    return "Unit-test metadata placement"_el;
}

auto MisplacedUnittestMetadataRule::description() const -> el::String {
    return "Detect test metadata macros attached to the wrong declaration."_el;
}

auto MisplacedUnittestMetadataRule::severity() const -> Severity {
    return Severity::High;
}

void MisplacedUnittestMetadataRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto isMetadata = [](const el::String &line) -> bool {
        return (line.startsWith("TAGS("_el) || line.startsWith("TESTED_TARGETS("_el) ||
                   line.startsWith("SKIP_BY_DEFAULT("_el)) &&
            line.contains(")"_el);
    };
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto line =
            source.slice(el::ByteRange{source.lineStart(lineIndex), source.lineEnd(lineIndex)}, true).trimmed();
        if (!isMetadata(line)) {
            continue;
        }
        if (!lineIndex.isZero() &&
            isMetadata(source
                    .slice(
                        el::ByteRange{
                            source.lineStart(lineIndex.decremented()), source.lineEnd(lineIndex.decremented())},
                        true)
                    .trimmed())) {
            continue;
        }
        const auto start = source.lineStart(lineIndex) +
            source.slice(el::ByteRange{source.lineStart(lineIndex), source.lineEnd(lineIndex)}, true)
                .find(line)
                .distanceFromZero();
        auto cursor = lineIndex;
        auto lastMetadata = lineIndex;
        auto next = el::String{};
        while (cursor.isWithin(source.lineCount())) {
            const auto currentLine =
                source.slice(el::ByteRange{source.lineStart(cursor), source.lineEnd(cursor)}, true).trimmed();
            if (isMetadata(currentLine)) {
                lastMetadata = cursor;
                const auto close = currentLine.find(")"_el);
                next = currentLine.slice(el::StringSide::Back, close.incremented()).trimmed();
                if (!next.isEmpty()) {
                    break;
                }
            } else if (!currentLine.isEmpty()) {
                next = currentLine;
                break;
            }
            ++cursor;
        }
        const auto metadataLine =
            source.slice(el::ByteRange{source.lineStart(lastMetadata), source.lineEnd(lastMetadata)}, true);
        const auto close = metadataLine.find(")"_el);
        const auto end = close.isNoIndex() ? source.lineEnd(lastMetadata)
                                           : source.lineStart(lastMetadata) + close.incremented().distanceFromZero();
        auto current = source.enclosingBlock(start);
        auto inFunction = false;
        while (current.isValid()) {
            const auto &block = source.blocks().getRef(current);
            inFunction |= block.kind != SourceBlock::Kind::Namespace && block.kind != SourceBlock::Kind::Class &&
                block.kind != SourceBlock::Kind::Struct;
            current = block.parent;
        }
        if (inFunction || source.slice(el::ByteRange{start, end}) != source.slice(el::ByteRange{start, end}, true) ||
            (!next.startsWith("class"_el) && !next.startsWith("struct"_el) && !next.startsWith("void"_el))) {
            candidates.append(el::ByteRange{start, end});
        }
        lineIndex = lastMetadata;
    }
}

}
