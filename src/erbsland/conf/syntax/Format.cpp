// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Format.hpp"

#include <algorithm>

namespace erbsland::conf::syntax {

Format::Format() {
    addRule(
        {Selector{}.setRelativeSection(true),
            FormatStylePatch{}.setSectionIndent(unit::CpCount{6}).setSectionSuffix(unit::CpCount{4})});
    addRule(
        {Selector{}.setMetaValue(true),
            FormatStylePatch{}
                .setValueNameCase(WordCase::Lower)
                .setSeparatorPositionMode(PositionMode::Spaces)
                .setSeparatorPosition(unit::CpCount{})});
}

auto Format::resolve(const Statement &statement) const -> FormatStyle {
    auto matches = std::vector<std::pair<std::array<int, 6>, const FormatStylePatch *>>{};
    for (const auto &rule : _rules) {
        if (auto score = rule.selector.match(statement)) {
            matches.emplace_back(*score, &rule.style);
        }
    }
    std::stable_sort(matches.begin(), matches.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    auto result = _base;
    for (const auto &match : matches) {
        match.second->applyTo(result);
    }
    return result;
}

auto Format::setBaseStyle(FormatStyle style) -> Format & {
    _base = std::move(style);
    return *this;
}

auto Format::addRule(FormatRule rule) -> Format & {
    _rules.push_back(std::move(rule));
    return *this;
}

auto Format::clearRules() noexcept -> Format & {
    _rules.clear();
    return *this;
}

}
