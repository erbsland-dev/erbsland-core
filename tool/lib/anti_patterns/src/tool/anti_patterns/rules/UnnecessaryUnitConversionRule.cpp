// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnnecessaryUnitConversionRule.hpp"

#include "../impl/TokenView.hpp"

#include <erbsland/RegEx.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto UnnecessaryUnitConversionRule::create() -> RulePtr {
    return std::make_unique<UnnecessaryUnitConversionRule>();
}

auto UnnecessaryUnitConversionRule::identifier() const -> el::String {
    return "unnecessary_unit_conversion"_el;
}

auto UnnecessaryUnitConversionRule::title() const -> el::String {
    return "Unnecessary unit conversions"_el;
}

auto UnnecessaryUnitConversionRule::description() const -> el::String {
    return "Detect conversions that discard and immediately restore a typed unit."_el;
}

auto UnnecessaryUnitConversionRule::severity() const -> Severity {
    return Severity::Low;
}

void UnnecessaryUnitConversionRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto tokens = impl::TokenView{source};
    static const auto cInteger = el::RegEx::compile(R"(^(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+)[uUlL]*$)"_el);
    for (auto index = el::ItemIndex::zero(); index.isWithin(tokens.count()); ++index) {
        if (!tokens.matches(index, {"."_el, "toSizeT"_el, "("_el, ")"_el}) &&
            !tokens.matches(index, {"->"_el, "toSizeT"_el, "("_el, ")"_el})) {
            continue;
        }
        const auto comparison = index.advanced(el::ItemCount{4});
        const auto &operation = tokens.at(comparison).text();
        if (operation != "=="_el && operation != "!="_el && operation != "<"_el && operation != ">"_el &&
            operation != "<="_el && operation != ">="_el) {
            continue;
        }
        const auto literal = comparison.incremented();
        if (!cInteger->findFirst(tokens.at(literal).text())) {
            continue;
        }
        // A larger right-hand expression can require native arithmetic; do not suggest a mechanical rewrite.
        const auto &next = tokens.at(literal.incremented()).text();
        if (!next.isEmpty() && next != ")"_el && next != ";"_el && next != ","_el && next != "&&"_el &&
            next != "||"_el && next != "?"_el && next != ":"_el && next != "}"_el) {
            continue;
        }
        candidates.append({tokens.at(index).position(), tokens.at(literal).range().endIndex()});
    }
}

}
