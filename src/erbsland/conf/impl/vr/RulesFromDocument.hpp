// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "Rule.hpp"
#include "Rules.hpp"

#include "../../Document.hpp"

#include <utility>
#include <vector>

namespace erbsland::conf::impl {

/// A context to process rules from a document.
class RulesFromDocument {

    /// The handler function.
    using HandlerFn = ConstraintPtr (*)(const ConstraintHandlerContext &context);
    /// One entry for a constraint handler.
    struct ConstraintHandler {
        text::String name;
        HandlerFn handler;
        bool acceptNot{true};
        bool acceptError{true};
    };
    /// A list of constraint handlers.
    using ConstraintHandlers = std::vector<ConstraintHandler>;

public:
    /// Create a new instance of this context.
    RulesFromDocument(RulesPtr rules, DocumentPtr document) :
        _rules{std::move(rules)}, _document(std::move(document)) {}

    // defaults and deletions
    ~RulesFromDocument() = default;
    RulesFromDocument(const RulesFromDocument &) = delete;
    auto operator=(const RulesFromDocument &) -> RulesFromDocument & = delete;

public:
    /// Read the rules from the document.
    void read();

private:
    /// Process the root document node.
    /// @param node The node to process.
    void processDocumentNode(const conf::ValuePtr &node);

    /// Process a regular node.
    /// A regular node can be a node with constraints, alternatives or implicit rule.
    /// @param node The node to process.
    void processRegularNode(const conf::ValuePtr &node);

    /// Process a node with rules (section with names).
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void processNodeRules(const conf::ValuePtr &node, const RulePtr &rule);

    /// Process the rules of a name-node.
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void processNameNodeRules(const conf::ValuePtr &node, const RulePtr &rule);

    /// Process the common (between regular and name node) node rules.
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void processCommonNodeRules(const conf::ValuePtr &node, const RulePtr &rule);

    /// Handle the `type` or `use_template` entry of a node rules definition.
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void handleTypeOrTemplate(const conf::ValuePtr &node, const RulePtr &rule);

    /// Handle the `case_sensitive` attribute of a rule.
    /// Ignores the attribute if not present.
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void handleCaseSensitive(const conf::ValuePtr &node, const RulePtr &rule);

    /// Handle one constraint and attribute of a node rules definition.
    /// @param customErrorMessages A map to collect custom error messages.
    /// @param rule The rule to fill-in.
    /// @param value The processed value.
    void handleConstraintAndAttributes(
        std::unordered_map<text::String, text::String> &customErrorMessages,
        const RulePtr &rule,
        const conf::ValuePtr &value);

    /// Process an implicitly defined rule (intermediate section).
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void processImplicitRules(const conf::ValuePtr &node, const RulePtr &rule);

    /// Process a node with alternatives (section list).
    /// @param node The node to process.
    /// @param rule The prepared rule to fill-in.
    void processAlternatives(const conf::ValuePtr &node, const RulePtr &rule);

    /// Process a template, when encountering a `use_template`.
    /// @param node The node that invoked the template.
    /// @param useTemplateValue The value that refers to the template.
    /// @param rule The prepared rule to fill-in.
    void processTemplate(const conf::ValuePtr &node, const conf::ValuePtr &useTemplateValue, const RulePtr &rule);

    /// Process a `vr_name` definition.
    /// @param node The node to process.
    void processNameNode(const conf::ValuePtr &node);

    /// Process a `vr_dependency` definition.
    /// @param node The node to process.
    void processDependencies(const conf::ValuePtr &node);

    /// Process a `vr_key` definition.
    /// @param node The node to process.
    void processKey(const conf::ValuePtr &node);

    /// Get the parent rule for the given node.
    /// @param node The node to get the parent rule for.
    /// @return The parent rule for the given node, or nullptr if none is found.
    [[nodiscard]] auto getParentRuleForNode(const conf::ValuePtr &node) const -> RulePtr;

    /// Create the rule name path.
    /// By default, this does not alter the name path at all.
    /// If `_pathForTemplate` is set, an initial `vr_template.<template-name>` is replaced with this path.
    [[nodiscard]] auto createRuleNamePath(const NamePath &namePath) const -> NamePath;

    /// Convert the name path of a node to the name path for the rule.
    /// 1. If `_pathForTemplate` is set, an initial `vr_template` is replaced with this path.
    /// 2. This does remove any indexes from the path, as these just mark alternatives and would prevent the
    /// path from matching the value in the document.
    /// 3. Also, if an element contains the `vr_vr_` prefix, this is replaced with `vr_`.
    /// @param namePath The original name path from the VR document.
    /// @return The name path for the rule.
    [[nodiscard]] auto createTargetNamePath(const NamePath &namePath) const -> NamePath;

    /// Test if the given name-path is a template path.
    [[nodiscard]] static auto isTemplatePath(const NamePath &namePath) -> bool;

    /// Only append regular names to the result.
    static void appendRegularNames(NamePath &result, const NamePath &namePath, std::size_t startIndex);

public: // constraint handlers.
    static auto handleDefault(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleDescription(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleError(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleIsOptional(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleIsSecret(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleTitle(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleVersion(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleMinimumVersion(const ConstraintHandlerContext &context) -> ConstraintPtr;
    static auto handleMaximumVersion(const ConstraintHandlerContext &context) -> ConstraintPtr;

private:
    [[nodiscard]] static auto resolveConstraintHandler(const text::String &name) -> const ConstraintHandler &;
    [[nodiscard]] static auto constraintHandlerTable() -> const ConstraintHandlers &;

private:
    /// The processed rule-set.
    RulesPtr _rules;
    /// The currently processed document.
    DocumentPtr _document;
    /// The name path to replace `vr_template`.
    /// Only used when traversing templates.
    /// It also acts as a flag if currently a template is processed.
    /// Cleared after template processing.
    NamePath _pathForTemplate;
};

}
