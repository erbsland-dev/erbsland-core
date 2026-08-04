// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rules.hpp"

#include "builder/Attributes.hpp"
#include "builder/Constraints.hpp"

#include "../impl/vr/RulesBuilder.hpp"

namespace erbsland::conf::vr {

/// A builder to create validation rules programmatically.
/// @tested{VrBuilderApiTest VrManualConstructionTest}
class RulesBuilder {
public:
    // defaults/deletions
    RulesBuilder() = default;
    ~RulesBuilder() = default;
    RulesBuilder(const RulesBuilder &) = delete;
    auto operator=(const RulesBuilder &) -> RulesBuilder & = delete;
    RulesBuilder(RulesBuilder &&) = delete;
    auto operator=(RulesBuilder &&) -> RulesBuilder & = delete;

public:
    /// Add a rule to the document.
    /// @param namePath The name-path of the new rule.
    /// @param ruleType The type of the new rule.
    /// @param attributes All the attributes for the rule definition.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void addRule(const NamePathLike &namePath, const RuleType ruleType, Attributes... attributes) {
        _builder.addRule(namePath, ruleType, attributes...);
    }

    /// Add an alternative to the document.
    /// @param namePath The name-path of the new rule.
    /// @param ruleType The type of the new rule.
    /// @param attributes All the attributes for the rule definition.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void addAlternative(const NamePathLike &namePath, const RuleType ruleType, Attributes... attributes) {
        _builder.addAlternative(namePath, ruleType, attributes...);
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
    impl::RulesBuilder _builder;
};

}
