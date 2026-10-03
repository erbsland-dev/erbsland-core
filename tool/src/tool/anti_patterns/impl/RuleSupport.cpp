// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RuleSupport.hpp"

#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>

#include <algorithm>

namespace tool::anti_patterns::impl {

using namespace el::text::literals;

auto namespaceType(const SourceIndex &source, const SourceBlock &block) -> bool {
    auto parent = source.parent(block);
    while (parent != nullptr) {
        if (parent->kind != SourceBlock::Kind::Namespace) {
            return false;
        }
        parent = source.parent(*parent);
    }
    return true;
}

auto namespaceName(const SourceIndex &source, const SourceBlock &block, const bool includeBlock) -> el::String {
    auto names = el::StringList{};
    if (includeBlock && block.kind == SourceBlock::Kind::Namespace && !block.name.isEmpty()) {
        names.append(block.name);
    }
    auto parent = source.parent(block);
    while (parent != nullptr) {
        if (parent->kind == SourceBlock::Kind::Namespace && !parent->name.isEmpty()) {
            names.append(parent->name);
        }
        parent = source.parent(*parent);
    }
    return names.reversed().join("::"_el);
}

auto matchesSourceDirectory(const SourceIndex &source, const el::String &name) -> bool {
    const auto parts = source.path().elements();
    if (parts.count() < el::ItemCount{2} || parts.get(el::ItemIndex{0}) != "src"_el ||
        parts.get(el::ItemIndex{1}) != "erbsland"_el) {
        return true;
    }
    if (!name.startsWith("erbsland"_el)) {
        return false;
    }
    auto domain = el::String{};
    if (parts.count() > el::ItemCount{3}) {
        domain = parts.get(el::ItemIndex{2});
    }
    if (domain.isEmpty()) {
        if (name.contains("::"_el)) {
            return false;
        }
    } else if (!name.startsWith(el::join("erbsland::"_el, domain))) {
        return false;
    }
    auto inImplPath = false;
    for (auto index = el::ItemIndex{3}; index.incremented().isWithin(parts.count()); ++index) {
        inImplPath |= parts.get(index) == "impl"_el;
    }
    return inImplPath == name.contains("::impl"_el);
}

auto standardExtension(const SourceIndex &source, const el::ByteIndex offset, const el::String &declaration) -> bool {
    auto snippets = el::StringList{declaration};
    const auto previousLine = source.lineIndex(offset).decremented();
    snippets.append(source.slice(el::ByteRange{source.lineStart(previousLine), source.statementEnd(offset)}, true));
    auto current = source.enclosingBlock(offset);
    auto inStd = false;
    while (current.isValid()) {
        const auto &block = source.blocks().getRef(current);
        if (block.kind == SourceBlock::Kind::Namespace && block.name == "std"_el) {
            inStd = true;
        }
        if (block.kind == SourceBlock::Kind::Class || block.kind == SourceBlock::Kind::Struct) {
            snippets.append(source.slice(el::ByteRange{block.headerStart, block.openingBrace()}, true));
        }
        current = block.parent;
    }
    for (const auto &snippet : snippets) {
        const auto specialization =
            (snippet.contains("formatter"_el) || snippet.contains("hash"_el)) && snippet.contains("<"_el);
        if (specialization && (inStd || snippet.contains("std::formatter"_el) || snippet.contains("std::hash"_el))) {
            return true;
        }
    }
    return false;
}

auto declarationPreamble(const el::String &line) -> bool {
    return line.startsWith("template <"_el) || line.startsWith("template<"_el) || line.startsWith("requires "_el) ||
        line.startsWith("requires("_el) || (line.startsWith("[["_el) && line.endsWith("]]"_el)) ||
        line.startsWith("TAGS("_el) || line.startsWith("TESTED_TARGETS("_el) || line.startsWith("SKIP_BY_DEFAULT("_el);
}

auto templateContinuation(const SourceIndex &source, const el::LineIndex lineIndex) -> bool {
    for (auto previous = lineIndex;; --previous) {
        const auto line =
            source.slice(el::ByteRange{source.lineStart(previous), source.lineEnd(previous)}, true).trimmed();
        if (line.startsWith("template <"_el) || line.startsWith("template<"_el)) {
            return true;
        }
        if (line.contains(";"_el) || line.contains("{"_el) || line.contains("}"_el) ||
            source.line(previous).trimmed().startsWith("///"_el)) {
            return false;
        }
        if (previous.isZero()) {
            break;
        }
    }
    return false;
}

auto constraintContinuation(const SourceIndex &source, const el::LineIndex lineIndex) -> bool {
    const auto current =
        source.slice(el::ByteRange{source.lineStart(lineIndex), source.lineEnd(lineIndex)}, true).trimmed();
    if (current.contains(";"_el)) {
        return false;
    }
    for (auto previous = lineIndex; !previous.isZero() && previous.absoluteDistanceTo(lineIndex) < el::LineCount{12};) {
        --previous;
        const auto line =
            source.slice(el::ByteRange{source.lineStart(previous), source.lineEnd(previous)}, true).trimmed();
        if (line.startsWith("requires"_el)) {
            return true;
        }
        if (line.contains(";"_el) || line.startsWith("template "_el) ||
            source.line(previous).trimmed().startsWith("///"_el)) {
            return false;
        }
    }
    return false;
}

auto hasApiDocumentation(const SourceIndex &source, const el::ByteIndex start) -> bool {
    auto lineIndex = source.lineIndex(start);
    while (!lineIndex.isZero()) {
        --lineIndex;
        const auto raw = source.line(lineIndex).trimmed();
        if (raw.startsWith("///"_el)) {
            return true;
        }
        const auto code =
            source.slice(el::ByteRange{source.lineStart(lineIndex), source.lineEnd(lineIndex)}, true).trimmed();
        if (!declarationPreamble(code) && !templateContinuation(source, lineIndex) &&
            !constraintContinuation(source, lineIndex)) {
            return false;
        }
    }
    return false;
}

auto outOfClassMember(const SourceIndex &source, const el::ByteIndex start) -> bool {
    if (!source.namespaceScope(start)) {
        return false;
    }
    const auto statement = source.slice(el::ByteRange{start, source.statementEnd(start)}, true).trimmed();
    if (!statement.endsWith("{"_el) || !statement.contains("::"_el) || !statement.contains("("_el) ||
        statement.startsWith("struct "_el) || statement.startsWith("class "_el)) {
        return false;
    }
    const auto opening = statement.find("("_el);
    if (opening.isNoIndex()) {
        return false;
    }
    auto lastQualifier = el::ByteIndex::noIndex();
    auto angleDepth = el::ItemCount{};
    for (auto index = el::ByteIndex::zero(); index.incremented() < opening; ++index) {
        const auto character = statement.charAt(index);
        if (character == U'<') {
            ++angleDepth;
        } else if (character == U'>' && !angleDepth.isZero()) {
            --angleDepth;
        } else if (character == U':' && angleDepth.isZero() && statement.charAt(index.incremented()) == U':') {
            lastQualifier = index;
            ++index;
        }
    }
    if (lastQualifier.isNoIndex()) {
        return false;
    }
    if (statement.slice(el::ByteRange{lastQualifier + el::ByteCount{2}, opening}).contains("<"_el)) {
        return false;
    }
    auto ownerEnd = lastQualifier;
    while (!ownerEnd.isZero() && statement.charAt(ownerEnd.decremented()).isAsciiWhitespace()) {
        --ownerEnd;
    }
    if (!ownerEnd.isZero() && statement.charAt(ownerEnd.decremented()) == U'>') {
        auto depth = el::ItemCount{};
        for (auto index = ownerEnd; !index.isZero(); --index) {
            const auto character = statement.charAt(index.decremented());
            if (character == U'>') {
                ++depth;
            } else if (character == U'<') {
                --depth;
                if (depth.isZero()) {
                    ownerEnd = index.decremented();
                    break;
                }
            }
        }
    }
    auto ownerStart = ownerEnd;
    while (!ownerStart.isZero() && statement.charAt(ownerStart.decremented()).isAsciiWord()) {
        --ownerStart;
    }
    return ownerStart < ownerEnd && statement.charAt(ownerStart).isAsciiUppercaseLetter();
}

}
