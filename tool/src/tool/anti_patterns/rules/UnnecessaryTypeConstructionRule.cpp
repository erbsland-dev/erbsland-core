// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnnecessaryTypeConstructionRule.hpp"

#include "../impl/TokenView.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto UnnecessaryTypeConstructionRule::create() -> RulePtr {
    return std::make_unique<UnnecessaryTypeConstructionRule>();
}

auto UnnecessaryTypeConstructionRule::identifier() const -> el::String {
    return "unnecessary_type_construction"_el;
}

auto UnnecessaryTypeConstructionRule::title() const -> el::String {
    return "Unnecessary explicit construction"_el;
}

auto UnnecessaryTypeConstructionRule::description() const -> el::String {
    return "Detect explicit type construction that can use the existing expression directly."_el;
}

auto UnnecessaryTypeConstructionRule::severity() const -> Severity {
    return Severity::Low;
}

void UnnecessaryTypeConstructionRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto tokens = impl::TokenView{source};
    for (auto index = el::ItemIndex::zero(); index.isWithin(tokens.count()); ++index) {
        const auto set = tokens.afterType(index, "CharSet"_el, "text"_el);
        if (tokens.at(set).text() == "{"_el) {
            const auto character = set.incremented();
            const auto opening = tokens.afterType(character, "Char"_el, "text"_el);
            const auto literal = opening.incremented();
            if (tokens.at(opening).text() == "{"_el && tokens.at(literal).kind() == el::cpp::CppToken::Kind::Literal &&
                tokens.at(literal).text().startsWith("U'"_el) && tokens.at(literal).text().endsWith("'"_el) &&
                tokens.matches(literal.incremented(), {"}"_el, "}"_el})) {
                candidates.append(
                    {tokens.at(character).position(), tokens.at(literal.incremented()).range().endIndex()});
            }
        }
        if (tokens.at(index).text() != "."_el && tokens.at(index).text() != "->"_el) {
            continue;
        }
        const auto method = index.incremented();
        if (tokens.at(method.incremented()).text() != "("_el) {
            continue;
        }
        const auto argument = method.advanced(el::ItemCount{2});
        const auto &name = tokens.at(method).text();
        if (name == "contains"_el || name == "startsWith"_el || name == "endsWith"_el || name == "find"_el) {
            const auto opening = tokens.afterType(argument, "String"_el, "text"_el);
            const auto literal = opening.incremented();
            if (tokens.at(opening).text() != "{"_el || tokens.at(literal).kind() != el::cpp::CppToken::Kind::Literal ||
                tokens.at(literal.incremented()).text() != "_el"_el ||
                tokens.at(literal).range().endIndex() != tokens.at(literal.incremented()).position() ||
                (!tokens.at(literal).text().startsWith("\""_el) && !tokens.at(literal).text().startsWith("R\""_el)) ||
                tokens.at(literal.advanced(el::ItemCount{2})).text() != "}"_el) {
                continue;
            }
            const auto after = literal.advanced(el::ItemCount{3});
            if (tokens.at(after).text() == ")"_el || tokens.at(after).text() == ","_el) {
                candidates.append({tokens.at(argument).position(), tokens.at(after.decremented()).range().endIndex()});
            }
        } else if (name == "slice"_el) {
            // Explicit coordinate constructors disambiguate ByteRange from CpRange after removing the range name.
            const auto opening = tokens.afterType(argument, "ByteRange"_el, "unit"_el);
            if (tokens.at(opening).text() != "{"_el) {
                continue;
            }
            const auto first = tokens.afterType(opening.incremented(), "ByteIndex"_el, "unit"_el);
            if (tokens.at(first).text() != "{"_el) {
                continue;
            }
            const auto firstEnd = tokens.closing(first);
            if (tokens.at(firstEnd.incremented()).text() != ","_el) {
                continue;
            }
            const auto second = firstEnd.advanced(el::ItemCount{2});
            auto secondOpening = tokens.afterType(second, "ByteCount"_el, "unit"_el);
            if (secondOpening.isNoIndex()) {
                secondOpening = tokens.afterType(second, "ByteIndex"_el, "unit"_el);
            }
            if (tokens.at(secondOpening).text() != "{"_el) {
                continue;
            }
            const auto secondEnd = tokens.closing(secondOpening);
            if (tokens.matches(secondEnd.incremented(), {"}"_el, ")"_el})) {
                candidates.append(
                    {tokens.at(argument).position(), tokens.at(secondEnd.incremented()).range().endIndex()});
            }
        }
    }
}

}
