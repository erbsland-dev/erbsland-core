// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Document Model".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrDocumentModelTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void requireNodeMarkedNotValidated(const std::string_view namePath) {
        auto value = document->valueOrThrow(el::text::String{namePath});
        const auto validationRule = value->validationRule();
        REQUIRE_NOT_EQUAL(validationRule, nullptr);
        REQUIRE_EQUAL(validationRule->type(), vr::RuleType::NotValidated);
    }

    void testValidDocument() {
        // Valid ELCL Document: A Validation Rules document must itself be a valid ELCL document and support all
        // features provided by the parser.
        // Validation rules use the same syntax and structural concepts as regular configuration documents.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "name = \"example\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "name = 123",
        }));
        WITH_CONTEXT(requireError("Expected a text value but got an integer value"));
        // Testing a failing ELCL document makes no sense here, as this is thoughtfully tested by the
        // parser tests.
    }

    void testSectionsDefineNodes() {
        // Sections Define Nodes: Each section in a Validation Rules document defines the rules for exactly one
        // Node in the validated document.
        // The section name represents the Name Path of the node being validated. When validation is applied to only
        // a subtree of a configuration, the name path is interpreted as relative to the validated root.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.name]",
            "type: \"text\"",
            "[server.port]",
            "type: \"integer\"",
            "[client.config.timeout]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[client]",
            "[.config]",
            "timeout: 0",
            "[server]",
            "port: 8080",
            "name: \"example\"",
        }));
        WITH_CONTEXT(requireFailLines({}));
        WITH_CONTEXT(requireError("expected a section with the name 'server'"));
    }

    void testImplicitAncestors() {
        // Implicit Ancestors: Elements of a name path that are not explicitly defined are treated
        // as implicit ancestor sections.
        // To reduce redundancy in validation rules documents, missing path elements implicitly create node-rules
        // definitions for required sections with names.
        WITH_CONTEXT(requireRulesPassLines({
            "[a.b.c.d.e.f.value]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[a.b.c.d.e.f]",
            "value: 123",
        }));
        WITH_CONTEXT(requirePassLines({
            "[a]",
            "[a.b]",
            "[a.b.c]",
            "[a.b.c.d]",
            "[a.b.c.d.e]",
            "[a.b.c.d.e.f]",
            "value: 123",
        }));
        WITH_CONTEXT(requirePassLines({
            "[a]",
            "[a.b]",
            "[a.b.c]",
            "[a.b.c.d]",
            "[a.b.c.d.e]",
            "[.f]",
            "value: 123",
        }));
    }

    void testExistenceRequirement() {
        // If a node-rules definition has no default value and is not explicitly marked as optional,
        // the corresponding node must exist in the configuration document.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "name: \"example\"",
        }));
        WITH_CONTEXT(requireFailLines({}));
        WITH_CONTEXT(requireError("expected a section with the name 'server'"));
        WITH_CONTEXT(requireFailLines({
            "[server]",
        }));
        WITH_CONTEXT(requireError("expected a text value with the name 'name'"));
    }

    void testClosedByDefault() {
        // Closed by Default: A configuration document must not contain nodes for which no matching
        // node-rules definition exists.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "name: \"example\"",
            "[client]",
        }));
        WITH_CONTEXT(requireError("Found an unexpected section in this document"));
    }

    void testNotValidatedBypassesClosedByDefaultForItsBranch() {
        // Closed by default exception: A NotValidated node defines an allowed branch that is ignored
        // during validation.
        WITH_CONTEXT(requireRulesPassLines({
            "[server]",
            "type: \"section\"",
            "[server.port]",
            "type: \"integer\"",
            "[server.plugin]",
            "type: \"NotValidated\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "port: 8080",
            "[.plugin]",
            "enabled: yes",
            "mode: \"debug\"",
            "[server.plugin.deep]",
            "value: 123",
        }));
        // The exception only applies inside the not-validated branch.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port: 8080",
            "unexpected: 42",
        }));
        WITH_CONTEXT(requireError("Found an unexpected integer value in this document"));
    }

    void testNotValidatedNodeIsNeitherRequiredNorForbidden() {
        // NotValidated nodes are optional by nature and may exist with arbitrary content.
        WITH_CONTEXT(requireRulesPassLines({
            "[server]",
            "type: \"section\"",
            "[server.name]",
            "type: \"text\"",
            "[server.plugin]",
            "type: \"NotValidated\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "name: \"example\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "name: \"example\"",
            "[.plugin]",
            "enabled: yes",
            "[server.plugin.settings]",
            "level: 3",
        }));
    }

    void testNotValidatedRuleIsAssignedToWholeIgnoredBranch() {
        // The validator must assign the NotValidated rule to every value in that branch.
        WITH_CONTEXT(requireRulesPassLines({
            "[server]",
            "type: \"section\"",
            "[server.plugin]",
            "type: \"NotValidated\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "[.plugin]",
            "enabled: yes",
            "[server.plugin.nested]",
            "name: \"demo\"",
        }));
        WITH_CONTEXT(requireNodeMarkedNotValidated("server.plugin"));
        WITH_CONTEXT(requireNodeMarkedNotValidated("server.plugin.enabled"));
        WITH_CONTEXT(requireNodeMarkedNotValidated("server.plugin.nested"));
        WITH_CONTEXT(requireNodeMarkedNotValidated("server.plugin.nested.name"));
    }
};
