// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Testing the 'contains' constraint based on the language documentation.
TESTED_TARGETS(ContainsConstraint)
TAGS(ValidationRules)
class VrContainsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // Type Matrix: `contains` and `not_contains` are only supported for text rules.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("contains: \"abc\"", {vr::RuleType::Text}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("not_contains: \"abc\"", {vr::RuleType::Text}));
    }

    void testUnsupportedConstraintValues() {
        // Value Type Rules: `contains` requires a single text or a list of texts.
        WITH_CONTEXT(requireOneConstraintFail("contains: 42", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'contains' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("contains: \"abc\", 42", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'contains' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("not_contains: no", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'not_contains' constraint must specify a single text value or a list of texts"));
    }

    void testContainsCaseInsensitiveByDefault() {
        // Case Sensitivity: Matching is case-insensitive unless explicitly enabled.
        WITH_CONTEXT(requireOneConstraintPass("contains: \"abc\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_ABC_suffix\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_xyz_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text does not contain \"abc\" (case-insensitive)"));
    }

    void testContainsCaseSensitive() {
        WITH_CONTEXT(requireOneConstraintPass("contains: \"abc\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_abc_suffix\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_ABC_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text does not contain \"abc\" (case-sensitive)"));
    }

    void testContainsWithMultipleValuesOrSemantics() {
        // Multiple values use OR semantics: any listed substring is sufficient.
        WITH_CONTEXT(requireOneConstraintPass("contains: \"abc\", \"xyz\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_abc_suffix\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_XYZ_suffix\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_123_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text does not contain \"abc\" or \"xyz\" (case-insensitive)"));
    }

    void testNotContainsCaseInsensitiveByDefault() {
        WITH_CONTEXT(requireOneConstraintPass("not_contains: \"secret\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"public_value\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_SECRET_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text must not contain \"secret\" (case-insensitive)"));
    }

    void testNotContainsCaseSensitive() {
        WITH_CONTEXT(requireOneConstraintPass("not_contains: \"secret\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_SECRET_suffix\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_secret_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text must not contain \"secret\" (case-sensitive)"));
    }

    void testNotContainsWithMultipleValues() {
        WITH_CONTEXT(requireOneConstraintPass("not_contains: \"abc\", \"xyz\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"prefix_123_suffix\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"prefix_XYZ_suffix\"",
        }));
        WITH_CONTEXT(requireError("The text must not contain \"abc\" or \"xyz\" (case-insensitive)"));
    }
};
