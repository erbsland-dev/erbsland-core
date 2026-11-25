// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StaticCastVoidRule.hpp"

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto StaticCastVoidRule::create() -> RulePtr {
    return std::make_unique<StaticCastVoidRule>();
}

auto StaticCastVoidRule::identifier() const -> el::String {
    return "static_cast_void"_el;
}

auto StaticCastVoidRule::title() const -> el::String {
    return "Discarded values"_el;
}

auto StaticCastVoidRule::description() const -> el::String {
    return "Detect static casts to void used to discard values."_el;
}

auto StaticCastVoidRule::severity() const -> Severity {
    return Severity::Medium;
}

void StaticCastVoidRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto &code = source.codeTokens();
    const auto &tokens = source.tokens();
    for (auto index = el::ItemIndex::zero(); index.advanced(el::ItemCount{4}).isWithin(code.count()); ++index) {
        const auto &first = tokens.getRef(code.getRef(index));
        if (first.text() != "static_cast"_el || tokens.getRef(code.getRef(index.incremented())).text() != "<"_el ||
            tokens.getRef(code.getRef(index.advanced(el::ItemCount{2}))).text() != "void"_el ||
            tokens.getRef(code.getRef(index.advanced(el::ItemCount{3}))).text() != ">"_el ||
            tokens.getRef(code.getRef(index.advanced(el::ItemCount{4}))).text() != "("_el) {
            continue;
        }
        const auto start = first.position();
        candidates.append(el::ByteRange{start, source.statementEnd(start)});
    }
}

}
