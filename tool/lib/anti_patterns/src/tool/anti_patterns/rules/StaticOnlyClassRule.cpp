// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StaticOnlyClassRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/CpCount.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto StaticOnlyClassRule::create() -> RulePtr {
    return std::make_unique<StaticOnlyClassRule>();
}

auto StaticOnlyClassRule::identifier() const -> el::String {
    return "static_only_class"_el;
}

auto StaticOnlyClassRule::title() const -> el::String {
    return "Classes containing only static members"_el;
}

auto StaticOnlyClassRule::description() const -> el::String {
    return "Detect classes used only as containers for static functions or constants."_el;
}

auto StaticOnlyClassRule::severity() const -> Severity {
    return Severity::High;
}

auto StaticOnlyClassRule::hasOnlyStaticMembers(
    const SourceIndex &source, const SourceBlock &block, const el::ItemIndex blockIndex) const -> bool {
    auto body = el::StringEditor{};
    auto foundStatic = false;
    auto position = block.openingBrace().incremented();
    for (const auto &child : source.blocks()) {
        if (child.parent != blockIndex || child.headerStart < position) {
            continue;
        }
        const auto header = source.slice(el::ByteRange{child.headerStart, child.openingBrace()}, true);
        if (child.kind == SourceBlock::Kind::Other) {
            if (!header.contains("static "_el)) {
                return false;
            }
            foundStatic = true;
        }
        body.append(source.slice(el::ByteRange{position, child.headerStart}, true));
        // Preserve source byte positions using one ASCII space per removed byte.
        body.append(
            U' ', el::CpCount::fromSizeT(child.headerStart.absoluteDistanceTo(child.braces.endIndex()).toSizeT()));
        position = child.braces.endIndex();
    }
    body.append(source.slice(el::ByteRange{position, block.closingBrace()}, true));
    const auto text = el::String{body};
    auto start = el::ByteIndex::zero();
    while (start.isWithin(text.length())) {
        const auto separator = text.find(";"_el, start);
        const auto end = separator.isNoIndex() ? el::ByteIndex::end(text.length()) : separator;
        auto member = text.slice(el::ByteRange{start, end}).trimmed();
        start = end.incremented();
        if (member.startsWith("public:"_el) || member.startsWith("protected:"_el) || member.startsWith("private:"_el)) {
            const auto colon = member.find(":"_el);
            member = member.slice(el::StringSide::Back, colon.incremented()).trimmed();
        }
        if (member.isEmpty() || member.startsWith("using "_el) || member.startsWith("typedef "_el) ||
            member.startsWith("friend "_el) || member.startsWith("static_assert"_el) ||
            member.startsWith("class "_el) || member.startsWith("struct "_el) || member.startsWith("enum "_el)) {
            continue;
        }
        if ((member.contains("= delete"_el) || member.contains("= default"_el)) &&
            (member.contains(block.name) || member.contains("operator="_el))) {
            if (member.contains("virtual"_el) && member.contains(el::join("~"_el, block.name))) {
                return false;
            }
            continue;
        }
        if (member.contains("static "_el)) {
            foundStatic = true;
            continue;
        }
        return false;
    }
    return foundStatic;
}

void StaticOnlyClassRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    source.blocks().forEach([&](const SourceBlock &block, const el::ItemIndex blockIndex) -> void {
        if ((block.kind != SourceBlock::Kind::Class && block.kind != SourceBlock::Kind::Struct) ||
            !impl::namespaceType(source, block) || block.name.isEmpty()) {
            return;
        }
        const auto header = source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true);
        const auto preambleStart = source.lineStart(source.lineIndex(block.headerStart).retreated(el::LineCount{4}));
        const auto preamble = source.slice(el::ByteRange{preambleStart, block.openingBrace()}, true);
        if (header.contains(":"_el) || (preamble.contains("requires("_el) && preamble.contains("::"_el))) {
            return;
        }
        if (hasOnlyStaticMembers(source, block, blockIndex)) {
            candidates.append(el::ByteRange{source.typeStart(block), block.openingBrace().incremented()});
        }
    });
}

}
