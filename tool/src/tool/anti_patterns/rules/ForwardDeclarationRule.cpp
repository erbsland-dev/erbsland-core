// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ForwardDeclarationRule.hpp"

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto ForwardDeclarationRule::create() -> RulePtr {
    return std::make_unique<ForwardDeclarationRule>();
}

auto ForwardDeclarationRule::identifier() const -> el::String {
    return "forward_declaration"_el;
}

auto ForwardDeclarationRule::title() const -> el::String {
    return "Forward declaration placement"_el;
}

auto ForwardDeclarationRule::description() const -> el::String {
    return "Detect forward declarations outside their canonical forward-declaration units."_el;
}

auto ForwardDeclarationRule::severity() const -> Severity {
    return Severity::High;
}

void ForwardDeclarationRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().name().endsWith("_fwd.hpp"_el)) {
        return;
    }
    const auto &code = source.codeTokens();
    const auto &tokens = source.tokens();
    for (auto index = el::ItemIndex::zero(); index.advanced(el::ItemCount{2}).isWithin(code.count()); ++index) {
        const auto &first = tokens.getRef(code.getRef(index));
        const auto &name = tokens.getRef(code.getRef(index.incremented()));
        const auto &end = tokens.getRef(code.getRef(index.advanced(el::ItemCount{2})));
        if ((first.text() == "class"_el || first.text() == "struct"_el) &&
            name.kind() == el::cpp::CppToken::Kind::Identifier && end.text() == ";"_el &&
            source.namespaceScope(first.position())) {
            candidates.append(el::ByteRange{first.position(), end.range().endIndex()});
        }
    }
}

}
