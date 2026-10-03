// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RulesBuilder.hpp"
#include "RulesBuilderBranch_fwd.hpp"

namespace erbsland::conf::vr {

/// A relative path view sharing a validation-rule builder backend.
/// Branches follow backend resets and may outlive their creator. Declare the parent rule before adding children or
/// configuring it. Calling reset() or takeRules() on a branch is a logic failure.
/// @seedoc{/reference/conf/validation_rules}
/// @tested{VrBuilderBranchTest}
class RulesBuilderBranch final : public RulesBuilder {
private:
    friend class RulesBuilder;

    /// Create a branch with an already resolved complete prefix.
    /// @param parent The builder whose backend is shared.
    /// @param parentPath The resolved complete path prefix.
    RulesBuilderBranch(const RulesBuilder &parent, NamePath parentPath);

public:
    // defaults/deletions
    ~RulesBuilderBranch() override = default;
    RulesBuilderBranch(const RulesBuilderBranch &) = delete;
    RulesBuilderBranch(RulesBuilderBranch &&) = delete;
    auto operator=(const RulesBuilderBranch &) -> RulesBuilderBranch & = delete;
    auto operator=(RulesBuilderBranch &&) -> RulesBuilderBranch & = delete;

public: // implement RulesBuilder
    [[nodiscard]] auto parentPath() const -> NamePath override;

protected: // implement RulesBuilder
    void configureRootImpl(std::span<const builder::Attribute *const> attributes) override;
    void addRuleImpl(
        const NamePathLike &namePath,
        RuleType ruleType,
        std::span<const builder::Attribute *const> attributes) override;
    void addAlternativeImpl(
        const NamePathLike &namePath,
        RuleType ruleType,
        std::span<const builder::Attribute *const> attributes) override;
    void resetImpl() override;
    auto takeRulesImpl() -> RulesPtr override;

private:
    /// Resolve and prepend this branch's prefix to a relative rule path.
    /// @param namePath The non-empty relative rule path.
    /// @return The complete rule path.
    [[nodiscard]] auto resolvePath(const NamePathLike &namePath) const -> NamePath;

private:
    NamePath _parentPath; ///< The complete path prefix.
};

}
