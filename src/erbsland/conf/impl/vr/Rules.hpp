// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DocumentValidator_fwd.hpp"
#include "Rule_fwd.hpp"
#include "Rules_fwd.hpp"

#include "../../vr/Rules.hpp"

namespace erbsland::conf::impl {

/// A rule tree constructed by definition identity and finalized before document validation.
/// @tested{VrRulePathsTest VrBuilderApiTest VrLiteralPathsTest}
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
    /// Attach a candidate to an alternatives container, creating the container if needed.
    /// The container retains the original role and value path; the candidate gains an alternative index.
    /// @param rule The unfinished candidate with its intended container paths and role.
    /// @throws ConfError (Validation) if the parent is missing or an incompatible rule exists.
    auto addAlternativeRule(const RulePtr &rule) -> void;
    /// Get the root rule.
    [[nodiscard]] auto root() const -> RulePtr;
    /// Follow a definition path from the root without decoding escapes.
    /// @param path The absolute expanded definition path, including alternative indexes.
    /// @param maxDepth Maximum components to follow; zero means the whole path.
    /// @return The rule, or an empty pointer for an empty or unresolved path.
    [[nodiscard]] auto ruleByDefinitionPath(const NamePath &path, std::size_t maxDepth = 0) const -> RulePtr;
    /// Validate the complete tree and resolve key paths before exposing rules to document validation.
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
