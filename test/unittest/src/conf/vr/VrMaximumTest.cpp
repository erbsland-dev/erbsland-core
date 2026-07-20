// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Testing the 'maximum' constraint using the validation-rules specification as source of truth.
TESTED_TARGETS(MaximumConstraint)
TAGS(ValidationRules)
class VrMaximumTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        WITH_CONTEXT(requireConstraintValidForRuleTypes(
            "maximum: 3",
            {vr::RuleType::Integer,
                vr::RuleType::Text,
                vr::RuleType::Bytes,
                vr::RuleType::ValueList,
                vr::RuleType::Section,
                vr::RuleType::SectionList,
                vr::RuleType::SectionWithTexts}));

        WITH_CONTEXT(requireConstraintValidForRuleTypes("maximum: 2.5", {vr::RuleType::Float}));

        WITH_CONTEXT(requireConstraintValidForRuleTypes("maximum: 2026-02-20", {vr::RuleType::Date}));

        WITH_CONTEXT(requireConstraintValidForRuleTypes("maximum: 2026-02-20 12:00:00z", {vr::RuleType::DateTime}));

        WITH_CONTEXT(requireConstraintValidForRuleTypes("maximum: 3, 4", {vr::RuleType::ValueMatrix}));
    }

    void testUnsupportedConstraintValues() {
        WITH_CONTEXT(requireOneConstraintFail("maximum: true", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'maximum' constraint for the 'text' rule must be of the type integer"));

        WITH_CONTEXT(requireOneConstraintFail("maximum: \"abc\"", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'maximum' constraint for the 'integer' rule must be of the type integer"));

        WITH_CONTEXT(requireOneConstraintFail("maximum: 2026-02-20", vr::RuleType::DateTime));
        WITH_CONTEXT(requireError("The 'maximum' constraint for the 'DateTime' rule must be of the type DateTime"));
    }

    void testMatrixConstraintDefinitionRequiresExactlyTwoIntegers() {
        WITH_CONTEXT(requireOneConstraintFail("maximum: 4", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(
            requireError("The 'maximum' constraint for a value matrix must be a list with two integer values"));

        WITH_CONTEXT(requireOneConstraintFail("maximum: 1, 2, 3", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(
            requireError("The 'maximum' constraint for a value matrix must be a list with two integer values"));

        WITH_CONTEXT(requireOneConstraintFail("maximum: 1, \"x\"", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(
            requireError("The 'maximum' constraint for a value matrix must be a list with two integer values"));
    }

    void testIntegerMaximumIsInclusive() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 42", vr::RuleType::Integer));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 41",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 42",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 43",
        }));
        WITH_CONTEXT(requireError("The value must be at most 42"));
    }

    void testTextUsesUnicodeCharacterCount() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 3", vr::RuleType::Text));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abc\"",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"😀😀😀\"",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"😀😀😀😀\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be at most 3"));
    }

    void testBytesUsesByteCount() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 3", vr::RuleType::Bytes));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <01 02 03>",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <01 02 03 04>",
        }));
        WITH_CONTEXT(requireError("The number of bytes must be at most 3"));
    }

    void testEntryCountInValueList() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2", vr::RuleType::ValueList));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 1, 2",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 1, 2, 3",
        }));
        WITH_CONTEXT(requireError("The number of values in this list must be at most 2"));
    }

    void testEntryCountInSectionList() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2", vr::RuleType::SectionList));

        WITH_CONTEXT(requirePassLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 2",
        }));

        WITH_CONTEXT(requireFailLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 2",
            "*[app.x]",
            "y: 3",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section list must be at most 2"));
    }

    void testEntryCountInSection() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2", vr::RuleType::Section));

        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "a: 1",
            "b: 2",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "a: 1",
            "b: 2",
            "c: 3",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be at most 2"));
    }

    void testEntryCountInSectionWithTexts() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2", vr::RuleType::SectionWithTexts));

        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 2",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 2",
            "\"c\": 3",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be at most 2"));
    }

    void testMatrixRowsAndColumnsAreChecked() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2, 3", vr::RuleType::ValueMatrix));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x:",
            "    * 10, 11, 12",
            "    * 20",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 10",
            "    * 20",
            "    * 30",
        }));
        WITH_CONTEXT(requireError("The number of rows in this value matrix must be at most 2"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 10, 11, 12, 13",
            "    * 20",
        }));
        WITH_CONTEXT(requireError("The number of columns in this row must be at most 3"));
    }

    void testMaximumOnFloatFiniteAndNegativeInfinity() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 10.0", vr::RuleType::Float));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 10.0",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -inf",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 10.1",
        }));
        WITH_CONTEXT(requireError("The value must be at most 10"));
    }

    void testMaximumOnFloatDisallowsNaN() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 10.0", vr::RuleType::Float));

        // Documented behavior: NaN is not allowed when any bound exists.
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: NaN",
        }));
    }

    void testMaximumOnFloatDisallowsPositiveInfinityOutsideBound() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 10.0", vr::RuleType::Float));

        // Documented behavior: +Inf is not allowed when it exceeds the maximum bound.
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: inf",
        }));
    }

    void testMaximumOnDate() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2026-02-20", vr::RuleType::Date));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 2026-02-20",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 2026-02-19",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 2026-02-21",
        }));
        WITH_CONTEXT(requireError("The date must be at most 2026-02-20"));
    }

    void testMaximumOnDateTime() {
        WITH_CONTEXT(requireOneConstraintPass("maximum: 2026-02-20 12:00:00z", vr::RuleType::DateTime));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 2026-02-20 12:00:00z",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 2026-02-20 11:59:59z",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 2026-02-20 12:00:01z",
        }));
        WITH_CONTEXT(requireError("The date-time must be at most 2026-02-20 12:00:00z"));
    }

    void testNegatedMaximum() {
        WITH_CONTEXT(requireOneConstraintPass("not_maximum: 10", vr::RuleType::Integer));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 11",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 10",
        }));
        WITH_CONTEXT(requireError("The value must be greater than 10"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 9",
        }));
        WITH_CONTEXT(requireError("The value must be greater than 10"));
    }

    void testMinimumMustNotExceedMaximum() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"integer\"",
            "minimum: 5",
            "maximum: 4",
        }));
        WITH_CONTEXT(requireError("minimum"));
        WITH_CONTEXT(requireError("maximum"));
    }
};
