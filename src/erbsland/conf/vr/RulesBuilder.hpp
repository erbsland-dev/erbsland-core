// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rules.hpp"
#include "RulesBuilderBranch_fwd.hpp"

#include "builder/Attributes.hpp"
#include "builder/Constraints.hpp"

#include "../impl/vr/RulesBuilder_fwd.hpp"
#include "../NamePath.hpp"

#include <array>
#include <concepts>
#include <memory>
#include <span>

namespace erbsland::conf::vr {

/// A builder to create validation rules programmatically.
/// @tested{VrBuilderApiTest VrManualConstructionTest VrBuilderBranchTest}
class RulesBuilder {
public:
    /// Create an empty validation-rule builder.
    RulesBuilder();
    /// Destroy the private builder implementation.
    virtual ~RulesBuilder();

    // defaults/deletions
    RulesBuilder(const RulesBuilder &) = delete;
    auto operator=(const RulesBuilder &) -> RulesBuilder & = delete;
    RulesBuilder(RulesBuilder &&) = delete;
    auto operator=(RulesBuilder &&) -> RulesBuilder & = delete;

protected:
    /// Share an existing builder backend with a branch builder.
    /// @param builder The non-null backend to share.
    explicit RulesBuilder(std::shared_ptr<impl::RulesBuilder> builder);

public: // branches
    /// Create a relative path branch without declaring any rules.
    /// @param namePath The non-empty path relative to this builder.
    /// @return A branch sharing this builder's backend.
    /// @throws ConfError (Validation) If the path contains text names or indexes, or is empty.
    [[nodiscard]] auto createBranch(const NamePathLike &namePath) -> RulesBuilderBranch;
    /// Get this builder's complete path prefix, or an empty path for the main builder.
    [[nodiscard]] virtual auto parentPath() const -> NamePath;

public: // rule construction
    /// Configure attributes of the implicit root or the existing branch parent rule.
    /// @tparam Attributes Builder-attribute types.
    /// @param attributes All attributes for the root rule definition.
    /// @throws ConfError (Validation) If the branch parent is missing or an attribute is invalid.
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
    /// @throws ConfError (Validation) If the rule path, type, attributes, or parent relationship is invalid.
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
    /// @throws ConfError (Validation) If the rule path, type, attributes, or parent relationship is invalid.
    template <typename... Attributes>
        requires(std::derived_from<Attributes, builder::Attribute> && ...)
    void addAlternative(const NamePathLike &namePath, const RuleType ruleType, Attributes... attributes) {
        const auto attributeList = std::array<const builder::Attribute *, sizeof...(Attributes)>{&attributes...};
        addAlternativeImpl(namePath, ruleType, attributeList);
    }

public: // document management
    /// Reset the builder and discard the current rules.
    /// Calling this method on a branch is a logic failure.
    void reset();

    /// Finalize the rules document and return the rules.
    /// Calling this method on a branch is a logic failure.
    /// This will finalize the currently processed rules document and return it to the caller.
    /// The builder is reset afterward and can be reused to create a new rules document.
    /// If errors are found while finalizing the rules document, an exception is thrown.
    /// @return The finalized rules document.
    /// @throws ConfError (Validation) on any logical error found. E.g. missing key references.
    [[nodiscard]] auto takeRules() -> RulesPtr;

protected: // implementation
    /// Configure the root using a type-erased attribute view.
    /// @param attributes The attributes for the implicit root or existing branch parent.
    /// @throws ConfError (Validation) If the branch parent is missing or an attribute is invalid.
    virtual void configureRootImpl(std::span<const builder::Attribute *const> attributes);
    /// Add a primary rule using a type-erased attribute view.
    /// @param namePath The rule path relative to this builder.
    /// @param ruleType The type accepted by the new rule.
    /// @param attributes The attributes for the rule definition.
    /// @throws ConfError (Validation) If the declaration or its parent relationship is invalid.
    virtual void addRuleImpl(
        const NamePathLike &namePath, RuleType ruleType, std::span<const builder::Attribute *const> attributes);
    /// Add an alternative rule using a type-erased attribute view.
    /// @param namePath The rule path relative to this builder.
    /// @param ruleType The type accepted by this alternative.
    /// @param attributes The attributes for the alternative definition.
    /// @throws ConfError (Validation) If the declaration or its parent relationship is invalid.
    virtual void addAlternativeImpl(
        const NamePathLike &namePath, RuleType ruleType, std::span<const builder::Attribute *const> attributes);
    /// Reset the shared backend; branches reject this document-level operation.
    virtual void resetImpl();
    /// Finalize the shared backend; branches reject this document-level operation.
    /// @return The completed rules document.
    /// @throws ConfError (Validation) If the completed rules definition is invalid.
    virtual auto takeRulesImpl() -> RulesPtr;

private:
    friend class RulesBuilderBranch;

    std::shared_ptr<impl::RulesBuilder> _builder; ///< The private builder implementation.
};

}
