// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Name.hpp"

#include <unordered_map>
#include <vector>

namespace erbsland::conf::impl {

class Rule;
using RulePtr = std::shared_ptr<Rule>;

/// A combined map and list of rules.
class RuleMap {
public:
    using List = std::vector<RulePtr>;
    using Map = std::unordered_map<Name, RulePtr>;

public:
    /// Create a new empty rule map.
    RuleMap() = default;

    // defaults
    ~RuleMap() = default;

public:
    /// Test if the rule map is empty.
    [[nodiscard]] auto empty() const noexcept -> bool;
    /// Get the size of the rule map.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Test if the rule map has a rule with the given name.
    /// @return True if the rule map has a rule with the given name, false otherwise.
    [[nodiscard]] auto hasRule(const Name &name) const noexcept -> bool;
    /// Get the rule with the given name.
    /// @return The rule with the given name or nullptr if not found.
    [[nodiscard]] auto rule(const Name &name) const -> RulePtr;
    /// Add a rule to this map.
    void addRule(const RulePtr &rule);
    /// Get an iterator over the map.
    [[nodiscard]] auto begin() const noexcept -> List::const_iterator;
    /// Get an iterator over the map.
    [[nodiscard]] auto end() const noexcept -> List::const_iterator;

private:
    List _ruleList; ///< The list with the rules.
    Map _ruleMap;   ///< A map with the rules.
};

}
