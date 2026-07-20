// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RuleMap.hpp"

#include "Rule.hpp"

namespace erbsland::conf::impl {

auto RuleMap::empty() const noexcept -> bool {
    return _ruleList.empty();
}

auto RuleMap::size() const noexcept -> std::size_t {
    return _ruleList.size();
}

auto RuleMap::hasRule(const Name &name) const noexcept -> bool {
    return _ruleMap.contains(name);
}

auto RuleMap::rule(const Name &name) const -> RulePtr {
    const auto it = _ruleMap.find(name);
    if (it == _ruleMap.end()) {
        return {};
    }
    return it->second;
}

void RuleMap::addRule(const RulePtr &rule) {
    _ruleList.push_back(rule);
    _ruleMap.emplace(rule->ruleName(), rule);
}

auto RuleMap::begin() const noexcept -> List::const_iterator {
    if (_ruleList.empty()) {
        return {};
    }
    return _ruleList.cbegin();
}

auto RuleMap::end() const noexcept -> List::const_iterator {
    if (_ruleList.empty()) {
        return {};
    }
    return _ruleList.cend();
}

}
