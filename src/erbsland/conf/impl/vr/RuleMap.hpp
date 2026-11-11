// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule_fwd.hpp"

#include "../../Name.hpp"

#include <unordered_map>
#include <vector>

namespace erbsland::conf::impl {

/// Children in declaration order, indexed by exact definition name or alternative index.
/// Value names can differ and are never keys in this map.
/// @tested{VrRulePathsTest}
class RuleMap {
public:
    /// Children in declaration order.
    using List = std::vector<RulePtr>;
    /// Definition-name lookup; names must remain unchanged after insertion.
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
    /// Test for a child with the exact definition name.
    /// @param name The definition name or alternative index; no decoding is performed.
    /// @return True if the rule map has a rule with the given name, false otherwise.
    [[nodiscard]] auto hasDefinitionName(const Name &name) const noexcept -> bool;
    /// Find a child by exact definition identity.
    /// @param name The definition name or alternative index; no decoding is performed.
    /// @return The rule with the given name or nullptr if not found.
    [[nodiscard]] auto ruleByDefinitionName(const Name &name) const -> RulePtr;
    /// Append and index a fully identified child.
    /// @param rule A non-null rule with a unique definition name, fixed before insertion.
    void addRule(const RulePtr &rule);
    /// Get an iterator over the map.
    [[nodiscard]] auto begin() const noexcept -> List::const_iterator;
    /// Get an iterator over the map.
    [[nodiscard]] auto end() const noexcept -> List::const_iterator;

private:
    List _ruleList; ///< The list with the rules.
    Map _ruleMap;   ///< Children keyed by definition name, including alternative indexes.
};

}
