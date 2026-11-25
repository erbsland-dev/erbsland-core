// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ErbslandCoreContainersStrictRule.hpp"

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto ErbslandCoreContainersStrictRule::create() -> RulePtr {
    return std::make_unique<ErbslandCoreContainersStrictRule>();
}

auto ErbslandCoreContainersStrictRule::identifier() const -> el::String {
    return "erbsland_core_containers_strict"_el;
}

auto ErbslandCoreContainersStrictRule::title() const -> el::String {
    return "Use Core containers"_el;
}

auto ErbslandCoreContainersStrictRule::description() const -> el::String {
    return "Detect standard-library containers where the project requires Core containers."_el;
}

auto ErbslandCoreContainersStrictRule::severity() const -> Severity {
    return Severity::Medium;
}

void ErbslandCoreContainersStrictRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto &code = source.codeTokens();
    const auto &tokens = source.tokens();
    for (auto index = el::ItemIndex::zero(); index.advanced(el::ItemCount{4}).isWithin(code.count()); ++index) {
        const auto &first = tokens.getRef(code.getRef(index));
        if (first.text() != "std"_el || tokens.getRef(code.getRef(index.incremented())).text() != "::"_el) {
            continue;
        }
        const auto &container = tokens.getRef(code.getRef(index.advanced(el::ItemCount{2}))).text();
        if ((container != "vector"_el && container != "set"_el && container != "map"_el) ||
            tokens.getRef(code.getRef(index.advanced(el::ItemCount{3}))).text() != "<"_el) {
            continue;
        }
        auto key = index.advanced(el::ItemCount{4});
        if (tokens.getRef(code.getRef(key)).text() == "el"_el || tokens.getRef(code.getRef(key)).text() == "text"_el ||
            tokens.getRef(code.getRef(key)).text() == "erbsland"_el) {
            ++key;
            if (!key.isWithin(code.count()) || tokens.getRef(code.getRef(key)).text() != "::"_el) {
                continue;
            }
            ++key;
            if (key.isWithin(code.count()) && tokens.getRef(code.getRef(key)).text() == "text"_el) {
                ++key;
                if (!key.isWithin(code.count()) || tokens.getRef(code.getRef(key)).text() != "::"_el) {
                    continue;
                }
                ++key;
            }
        }
        if (!key.isWithin(code.count()) || tokens.getRef(code.getRef(key)).text() != "String"_el) {
            continue;
        }
        if (!key.incremented().isWithin(code.count())) {
            continue;
        }
        const auto &after = tokens.getRef(code.getRef(key.incremented())).text();
        if (after != ">"_el && after != ","_el) {
            continue;
        }
        candidates.append(el::ByteRange{first.position(), tokens.getRef(code.getRef(key)).range().endIndex()});
    }
}

}
