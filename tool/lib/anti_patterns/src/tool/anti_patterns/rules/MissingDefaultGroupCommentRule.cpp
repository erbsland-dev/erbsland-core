// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MissingDefaultGroupCommentRule.hpp"

#include "../impl/RuleSupport.hpp"
#include "../SourceIndex.hpp"

#include <erbsland/StringJoins.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto MissingDefaultGroupCommentRule::create() -> RulePtr {
    return std::make_unique<MissingDefaultGroupCommentRule>();
}

auto MissingDefaultGroupCommentRule::identifier() const -> el::String {
    return "missing_default_group_comment"_el;
}

auto MissingDefaultGroupCommentRule::title() const -> el::String {
    return "Missing defaults group comment"_el;
}

auto MissingDefaultGroupCommentRule::description() const -> el::String {
    return "Detect defaulted or deleted members without the required group comment."_el;
}

auto MissingDefaultGroupCommentRule::severity() const -> Severity {
    return Severity::Medium;
}

void MissingDefaultGroupCommentRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().suffix() != ".hpp"_el || source.path().name().endsWith("_fwd.hpp"_el)) {
        return;
    }
    for (auto lineIndex = el::LineIndex::zero(); lineIndex.isWithin(source.lineCount()); ++lineIndex) {
        const auto lineStart = source.lineStart(lineIndex);
        const auto compact = source.slice(el::ByteRange{lineStart, source.lineEnd(lineIndex)}, true).trimmed();
        if (!compact.contains("= default"_el) && !compact.contains("= delete"_el)) {
            continue;
        }
        if (!compact.contains("("_el) || !compact.endsWith(";"_el)) {
            continue;
        }
        const auto start = lineStart +
            source.slice(el::ByteRange{lineStart, source.lineEnd(lineIndex)}, true).find(compact).distanceFromZero();
        const auto enclosing = source.enclosingBlock(start);
        if (enclosing.isNoIndex()) {
            continue;
        }
        const auto &type = source.blocks().getRef(enclosing);
        if (type.kind != SourceBlock::Kind::Class && type.kind != SourceBlock::Kind::Struct) {
            continue;
        }
        if (!compact.startsWith(el::join(type.name, "("_el)) &&
            !compact.startsWith(el::join("~"_el, type.name, "("_el)) && !compact.contains("operator = ("_el) &&
            !compact.contains("operator=("_el)) {
            continue;
        }
        if (compact.contains(el::join(type.name, "()"_el)) && compact.contains("= default"_el) &&
            impl::hasApiDocumentation(source, start)) {
            continue;
        }
        auto grouped = false;
        for (auto previous = lineIndex; !previous.isZero();) {
            --previous;
            const auto raw = source.line(previous).trimmed();
            const auto code =
                source.slice(el::ByteRange{source.lineStart(previous), source.lineEnd(previous)}, true).trimmed();
            if (raw.endsWith("// defaults"_el) || raw.endsWith("// defaults/deletions"_el) ||
                raw.endsWith("// defaults."_el) || raw.endsWith("// defaults/deletions."_el)) {
                grouped = true;
                break;
            }
            if (code.isEmpty() || code.contains("= default"_el) || code.contains("= delete"_el) ||
                impl::declarationPreamble(code) || code.startsWith("requires "_el)) {
                continue;
            }
            break;
        }
        if (!grouped) {
            candidates.append(el::ByteRange{start, source.statementEnd(start)});
        }
    }
}

}
