// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DocumentValidator_fwd.hpp"
#include "Rule_fwd.hpp"
#include "Rules_fwd.hpp"

#include "../../vr/Rules.hpp"

namespace erbsland::conf::impl {

/// A set of validation rules.
class Rules : public vr::Rules {
public:
    /// Create an empty set of rules.
    Rules();

    // defaults
    ~Rules() override = default;

public: // public interface
    void validate(const conf::ValuePtr &value, Integer version) override;

public: // implementation interface
    /// Test if there are no rules defined.
    [[nodiscard]] auto empty() const -> bool;
    /// Add a rule to this set by attaching it to the correct parent.
    /// @param rule The rule to add.
    auto addRule(const RulePtr &rule) -> void;
    /// Add an alternative.
    /// Compared with `addRule`, if the last element in the name path does not exist, it will be created
    /// as an alternative with the rule added as the first element.
    /// Subsequent calls to `addAlternativeRule` will add the rule as an alternative to the last element
    /// in the name path.
    auto addAlternativeRule(const RulePtr &rule) -> void;
    /// Get the root rule.
    [[nodiscard]] auto root() const -> RulePtr;
    /// Get the rule at the given path.
    /// @param path The name path of the rule to get.
    /// @param maxDepth The maximum depth to search for the rule.
    /// @return The rule at the given path, or nullptr if not found.
    [[nodiscard]] auto ruleForNamePath(const NamePath &path, std::size_t maxDepth = 0) const -> RulePtr;
    /// Validate this rules definition for correctness.
    /// @throws ConfError (Validation) for any invalid rule definition.
    void validateDefinition();

public: // tests
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Rules &rule) -> InternalViewPtr;
    friend auto internalView(const RulesPtr &rule) -> InternalViewPtr;
#endif

private:
    friend class DocumentValidator;

    /// Test whether an error points to a value marked as secret.
    [[nodiscard]] static auto isSecretErrorValue(const conf::ValuePtr &root, const ConfError &error) noexcept -> bool;

private:
    RulePtr _root; ///< The root rule of this set. Always a `Section` rule with no constraints.
};

}
