// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rules.hpp"

#include "builder/Attributes.hpp"
#include "builder/Constraints.hpp"

#include "../impl/vr/RulesBuilder_fwd.hpp"

#include <array>
#include <concepts>
#include <memory>
#include <span>

namespace erbsland::conf::vr {

/// A builder to create validation rules programmatically.
/// @tested{VrBuilderApiTest VrManualConstructionTest}
class RulesBuilder {
public:
    /// Create an empty validation-rule builder.
    RulesBuilder();
    /// Destroy the private builder implementation.
    ~RulesBuilder();

    // defaults/deletions
    RulesBuilder(const RulesBuilder &) = delete;
    auto operator=(const RulesBuilder &) -> RulesBuilder & = delete;
    RulesBuilder(RulesBuilder &&) = delete;
    auto operator=(RulesBuilder &&) -> RulesBuilder & = delete;

public:
    /// Configure attributes of the implicit root section rule.
    /// @tparam Attributes Builder-attribute types.
    /// @param attributes All attributes for the root rule definition.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void configureRoot(Attributes... attributes) {
        const auto attributeList = std::array<const builder::Attribute *, sizeof...(Attributes)>{&attributes...};
        configureRootImpl(attributeList);
    }

    /// Add a rule to the document.
    /// @tparam Attributes Builder-attribute types.
    /// @param namePath The name-path of the new rule.
    /// @param ruleType The type of the new rule.
    /// @param attributes All the attributes for the rule definition.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void addRule(const NamePathLike &namePath, const RuleType ruleType, Attributes... attributes) {
        const auto attributeList = std::array<const builder::Attribute *, sizeof...(Attributes)>{&attributes...};
        addRuleImpl(namePath, ruleType, attributeList);
    }

    /// Add an alternative to the document.
    /// @tparam Attributes Builder-attribute types.
    /// @param namePath The name-path of the new rule.
    /// @param ruleType The type of the new rule.
    /// @param attributes All the attributes for the rule definition.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void addAlternative(const NamePathLike &namePath, const RuleType ruleType, Attributes... attributes) {
        const auto attributeList = std::array<const builder::Attribute *, sizeof...(Attributes)>{&attributes...};
        addAlternativeImpl(namePath, ruleType, attributeList);
    }

public:
    /// Reset the builder and discard the current rules
    void reset();

    /// Finalize the rules document and return the rules.
    /// This will finalize the currently processed rules document and return it to the caller.
    /// The builder is reset afterward and can be reused to create a new rules document.
    /// If errors are found while finalizing the rules document, an exception is thrown.
    /// @return The finalized rules document.
    /// @throws ConfError (Validation) on any logical error found. E.g. missing key references.
    [[nodiscard]] auto takeRules() -> RulesPtr;

private:
    /// Configure the root using a type-erased attribute view.
    void configureRootImpl(std::span<const builder::Attribute *const> attributes);
    /// Add a primary rule using a type-erased attribute view.
    void addRuleImpl(
        const NamePathLike &namePath, RuleType ruleType, std::span<const builder::Attribute *const> attributes);
    /// Add an alternative rule using a type-erased attribute view.
    void addAlternativeImpl(
        const NamePathLike &namePath, RuleType ruleType, std::span<const builder::Attribute *const> attributes);

private:
    std::unique_ptr<impl::RulesBuilder> _builder; ///< The private builder implementation.
};

}
