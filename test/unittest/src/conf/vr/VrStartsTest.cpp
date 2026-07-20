// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing the 'starts' constraint.
TESTED_TARGETS(StartsConstraint)
TAGS(ValidationRules)
class VrStartsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // Type Matrix: starts is supported only for text rules.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("starts: \"id\"", {vr::RuleType::Text}));
    }

    void testUnsupportedConstraintValues() {
        // Rule value must be a single text value or a list of text values.
        requireInvalidStartsValue("starts: 42");
        requireInvalidStartsValue("starts: true");
        requireInvalidStartsValue("starts: 0.5");
        requireInvalidStartsValue("starts: <01 02>");
        requireInvalidStartsValue("starts: 2026-01-01");
        requireInvalidStartsValue("starts: 10 minutes");
        requireInvalidStartsValue("starts: /abc/");
        requireInvalidStartsValue("starts: \"id\", 7");
    }

    void testStartsSingleValueDefaultCaseInsensitive() {
        // Default behavior: text comparison is case-insensitive.
        WITH_CONTEXT(requireOneConstraintPass("starts: \"server_\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"SERVER_01\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"client_01\"",
        }));
        WITH_CONTEXT(requireError("The text does not start with \"server_\""));
        WITH_CONTEXT(requireError("case-insensitive"));
    }

    void testStartsCaseSensitive() {
        // Explicit case-sensitive mode must be supported.
        WITH_CONTEXT(requireOneConstraintPass("starts: \"server_\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"server_01\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"SERVER_01\"",
        }));
        WITH_CONTEXT(requireError("The text does not start with \"server_\""));
        WITH_CONTEXT(requireError("case-sensitive"));
    }

    void testStartsMultipleValuesUsesOrSemantics() {
        // Multiple Values: any listed prefix must satisfy the constraint.
        WITH_CONTEXT(requireOneConstraintPass("starts: \"server_\", \"client_\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"server_01\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"CLIENT_01\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"api_01\"",
        }));
        WITH_CONTEXT(requireError("The text does not start with \"server_\" or \"client_\""));
    }

    void testNotStartsSingleValue() {
        // Negation: not_starts inverts the result of starts.
        WITH_CONTEXT(requireOneConstraintPass("not_starts: \"tmp_\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prod_01\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"TMP_01\"",
        }));
        WITH_CONTEXT(requireError("The text must not start with \"tmp_\""));
        WITH_CONTEXT(requireError("case-insensitive"));
    }

    void testNotStartsMultipleValuesUsesOrSemantics() {
        WITH_CONTEXT(requireOneConstraintPass("not_starts: \"server_\", \"client_\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"api_01\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"client_01\"",
        }));
        WITH_CONTEXT(requireError("The text must not start with \"server_\" or \"client_\""));
    }

    void testNoMixingPositiveAndNegativeForms() {
        // Positive and negated forms must not be combined in one rule node.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"text\"",
            "starts: \"id_\"",
            "not_starts: \"tmp_\"",
        }));
        WITH_CONTEXT(requireError("must not mix positive and negative constraints for the same type"));
    }

    void testCustomErrorMessageForStarts() {
        // `_error` message replaces the default text for this constraint.
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "starts: \"id_\"",
            "starts_error: \"Identifier must begin with id_\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"tmp_42\"",
        }));
        WITH_CONTEXT(requireError("Identifier must begin with id_"));
        REQUIRE_FALSE(lastError.contains(
            el::text::String{"does not start with"}, el::text::cCaseInsensitive.asciiComparisonFn()));
    }

    void testCustomErrorMessageForNotStarts() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "not_starts: \"tmp_\"",
            "not_starts_error: \"Temporary prefix is forbidden\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"tmp_42\"",
        }));
        WITH_CONTEXT(requireError("Temporary prefix is forbidden"));
        REQUIRE_FALSE(lastError.contains(
            el::text::String{"must not start with"}, el::text::cCaseInsensitive.asciiComparisonFn()));
    }

    void testCustomErrorRequiresExistingConstraint() {
        // `_error` entries require the corresponding constraint.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"text\"",
            "starts_error: \"Message\"",
        }));
        WITH_CONTEXT(requireError("There is no constraint 'starts' for the custom error message 'starts_error'"));
    }

    void testCustomErrorNeedsExactConstraintName() {
        // Exact Name Matching: starts_error does not map to not_starts.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"text\"",
            "not_starts: \"tmp_\"",
            "starts_error: \"Message\"",
        }));
        WITH_CONTEXT(requireError("There is no constraint 'starts' for the custom error message 'starts_error'"));
    }

private:
    void requireInvalidStartsValue(const std::string &constraintLine) {
        WITH_CONTEXT(requireOneConstraintFail(constraintLine, vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'starts' constraint must specify a single text value or a list of texts"));
    }
};
