// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Tests for the 'ends' and 'not_ends' constraints.
TESTED_TARGETS(EndsConstraint)
TAGS(ValidationRules)
class VrEndsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // `ends` is only supported for text rules.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("ends: \"_id\"", {vr::RuleType::Text}));
    }

    void testUnsupportedValueTypes() {
        // Only a single text or a list of texts is accepted.
        WITH_CONTEXT(requireOneConstraintFail("ends: 42", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'ends' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("ends: 2026-01-01", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'ends' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("ends: \"_id\", 1", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'ends' constraint must specify a single text value or a list of texts"));
    }

    void testEndsCaseInsensitiveByDefault() {
        WITH_CONTEXT(requireOneConstraintPass("ends: \"_id\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"example_ID\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"example_name\"",
        }));
        WITH_CONTEXT(requireError("The text does not end with \"_id\" (case-insensitive)"));
    }

    void testEndsCaseSensitive() {
        WITH_CONTEXT(requireOneConstraintPass("ends: \"_id\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"example_id\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"example_ID\"",
        }));
        WITH_CONTEXT(requireError("The text does not end with \"_id\" (case-sensitive)"));
    }

    void testEndsWithMultipleValuesOrSemantics() {
        WITH_CONTEXT(requireOneConstraintPass("ends: \"_id\", \"_name\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"example_name\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"example_id\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"example_tag\"",
        }));
        WITH_CONTEXT(requireError("The text does not end with \"_id\" or \"_name\" (case-insensitive)"));
    }

    void testNotEnds() {
        WITH_CONTEXT(requireOneConstraintPass("not_ends: \"_tmp\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"cache_id\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"cache_tmp\"",
        }));
        WITH_CONTEXT(requireError("The text must not end with \"_tmp\" (case-insensitive)"));
    }

    void testNoMixOfEndsAndNotEnds() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"text\"",
            "ends: \"_id\"",
            "not_ends: \"_tmp\"",
        }));
        WITH_CONTEXT(requireError("must not mix positive and negative constraints for the same type"));
    }

    void testCustomConstraintError() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "ends: \"_id\"",
            "ends_error: \"Identifier must end in _id\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"example_name\"",
        }));
        WITH_CONTEXT(requireError("Identifier must end in _id"));
    }
};
