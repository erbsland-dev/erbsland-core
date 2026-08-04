// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Alternatives".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrAlternativesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    // Rule 1: Section List Required: Alternatives must be defined using a section list.
    // We cannot test this rule because it is enforced by the parser and not by the validation rules.

    void testCompleteDefinitions() {
        // #2: Complete Definitions: Each alternative must be a complete and valid node-rules definition.
        WITH_CONTEXT(requireRulesFailLines({
            "*[app.threads]*",
            "type: \"integer\"",
            "*[app.threads]*",
            "minimum: 20",
        }));
        WITH_CONTEXT(requireError("either a 'type' or a 'use_template'"));
    }

    void testAlternativeSelectionByConstraints() {
        // #3: Order of Definition + First Match Wins: The first alternative that fulfills its own constraints is
        // selected.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.threads]*",
            "type: \"integer\"",
            "minimum: 10",
            "*[app.threads]*",
            "type: \"integer\"",
            "minimum: 1",
        }));

        // Matches the first alternative.
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "threads: 15",
        }));

        // Does not fulfill the first alternative, but must match the second.
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "threads: 5",
        }));
    }

    void testMissingRequiredNodeListsAllValidTypes() {
        // Missing Required Node: Missing nodes must list all valid types for matching alternatives.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.service]*",
            "type: \"integer\"",
            "*[app.service]*",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
        }));
        WITH_CONTEXT(requireError("integer value"));
        WITH_CONTEXT(requireError("text value"));
    }

    void testNoAlternativeMatchesListsTypes() {
        // ConfError Handling When No Alternative Matches: If no alternative matches, list all valid types.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.service]*",
            "type: \"integer\"",
            "*[app.service]*",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "service: 1, 2",
        }));
        WITH_CONTEXT(requireError("integer value"));
        WITH_CONTEXT(requireError("text value"));
    }

    void testSingleMatchingAlternativeUsesRegularErrors() {
        // If exactly one alternative matches, handle errors like a regular node-rules definition.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.service]*",
            "type: \"integer\"",
            "minimum: 10",
            "*[app.service]*",
            "type: \"text\"",
            "starts: \"service:\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "service: 5",
        }));
        WITH_CONTEXT(requireError("at least 10"));
    }

    void testMultipleMatchingAlternativesUseFirstForErrors() {
        // If multiple alternatives match, use the first matching alternative for error reporting.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.threads]*",
            "type: \"integer\"",
            "minimum: 10",
            "*[app.threads]*",
            "type: \"integer\"",
            "maximum: 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "threads: 5",
        }));
        WITH_CONTEXT(requireError("at least 10"));

        WITH_CONTEXT(requireRulesPassLines({
            "*[app.threads]*",
            "type: \"integer\"",
            "minimum: 10",
            "minimum_error: \"tag_a\"",
            "*[app.threads]*",
            "type: \"integer\"",
            "maximum: 2",
            "maximum_error: \"tag_b\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "threads: 5",
        }));
        WITH_CONTEXT(requireError("tag_a"));

        WITH_CONTEXT(requireRulesPassLines({
            "*[app.threads]*",
            "type: \"integer\"",
            "minimum: 10",
            "error: \"tag_a\"",
            "*[app.threads]*",
            "type: \"integer\"",
            "maximum: 2",
            "error: \"tag_b\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "threads: 5",
        }));
        WITH_CONTEXT(requireError("tag_a"));
    }

    void testDefaultsAcrossAlternatives() {
        // Defaults: The first default encountered is used; multiple defaults are an error.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.service]*",
            "type: \"integer\"",
            "default: 10",
            "*[app.service]*",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
        }));
        const auto service = document->getOrThrow<Integer>(el::text::String{"app.service"});
        REQUIRE_EQUAL(service, 10);

        WITH_CONTEXT(requireRulesFailLines({
            "*[app.service]*",
            "type: \"integer\"",
            "default: 10",
            "*[app.service]*",
            "type: \"text\"",
            "default: \"https\"",
        }));
        WITH_CONTEXT(requireError("default"));
    }

    void testOptionalityAcrossAlternatives() {
        // Optionality: If any alternative defines is_optional, the node is optional.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.service]*",
            "type: \"integer\"",
            "is_optional: yes",
            "*[app.service]*",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
        }));

        // is_optional must be in the first alternative.
        WITH_CONTEXT(requireRulesFailLines({
            "*[app.service]*",
            "type: \"integer\"",
            "*[app.service]*",
            "type: \"text\"",
            "is_optional: yes",
        }));
        WITH_CONTEXT(requireError("first alternative"));

        // is_optional must not be defined multiple times.
        WITH_CONTEXT(requireRulesFailLines({
            "*[app.service]*",
            "type: \"integer\"",
            "is_optional: yes",
            "*[app.service]*",
            "type: \"text\"",
            "is_optional: yes",
        }));
        WITH_CONTEXT(requireError("optional"));
    }

    void testChildValidationAfterSelection() {
        // Sections in Alternatives: Child nodes are validated only after an alternative is selected.
        WITH_CONTEXT(requireRulesPassLines({
            "*[app.screen]*",
            "type: \"section\"",
            "[app.screen.size]",
            "type: \"integer\"",
            "*[app.screen]*",
            "type: \"section\"",
            "[app.screen.width]",
            "type: \"integer\"",
            "*[app.screen]*",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.screen]",
            "width: 10",
        }));
        WITH_CONTEXT(requireError("unexpected integer value"));
    }
};
