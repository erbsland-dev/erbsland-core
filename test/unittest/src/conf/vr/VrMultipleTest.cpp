// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Testing the 'multiple' constraint.
TESTED_TARGETS(MultipleConstraint)
TAGS(ValidationRules)
class VrMultipleTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // Type matrix from the documentation.
        WITH_CONTEXT(requireConstraintValidForRuleTypes(
            "multiple: 2",
            {vr::RuleType::Integer,
                vr::RuleType::Text,
                vr::RuleType::Bytes,
                vr::RuleType::ValueList,
                vr::RuleType::Section,
                vr::RuleType::SectionList,
                vr::RuleType::SectionWithTexts}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("multiple: 0.1", {vr::RuleType::Float}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("multiple: 2, 3", {vr::RuleType::ValueMatrix}));
    }

    void testUnsupportedConstraintValues() {
        // Unsupported value types for each supported rule type.
        WITH_CONTEXT(requireOneConstraintFail("multiple: true", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'multiple' constraint for an integer rule must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 2", vr::RuleType::Float));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a float rule must be a float"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a text rule must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::Bytes));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a bytes rule must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::ValueList));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a value list must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 2", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must be a list with two integer values"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 2, 3, 4", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must be a list with two integer values"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::Section));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a section or section list must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::SectionList));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a section or section list must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.1", vr::RuleType::SectionWithTexts));
        WITH_CONTEXT(requireError("The 'multiple' constraint for a section or section list must be an integer"));
    }

    void testDivisorMustNotBeZero() {
        WITH_CONTEXT(requireOneConstraintFail("multiple: 0", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("must not be zero"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0.0", vr::RuleType::Float));
        WITH_CONTEXT(requireError("must not be zero"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 0, 2", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must not be zero"));

        WITH_CONTEXT(requireOneConstraintFail("multiple: 2, 0", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must not be zero"));
    }

    void testIntegerMultiplesAndNegatives() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 8", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 16",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -16",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 14",
        }));
        WITH_CONTEXT(requireError("The value must be a multiple of 8"));

        WITH_CONTEXT(requireOneConstraintPass("multiple: -8", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -16",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 10",
        }));
        WITH_CONTEXT(requireError("The value must be a multiple of -8"));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 8", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 14",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: -16",
        }));
        WITH_CONTEXT(requireError("The value must not be a multiple of 8"));
    }

    void testFloatMultiples() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 0.1", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.9",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -0.8",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.95",
        }));
        WITH_CONTEXT(requireError("must be a multiple of 0.1"));

        WITH_CONTEXT(requireOneConstraintPass("multiple: -0.1", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.9",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.95",
        }));
        WITH_CONTEXT(requireError("must be a multiple of -0.1"));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 0.1", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.95",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.9",
        }));
        WITH_CONTEXT(requireError("must not be a multiple of 0.1"));
    }

    void testTextCharacterLength() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 3", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"abcd\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be a multiple of 3"));

        // Unicode code points count as characters regardless of UTF-8 byte length.
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"äöü\"",
        }));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 3", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abcd\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requireError("must not be a multiple of 3"));
    }

    void testBytesLength() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 4", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <00 01 02 03>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <00 01 02>",
        }));
        WITH_CONTEXT(requireError("The number of bytes must be a multiple of 4"));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 4", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <00 01 02>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <00 01 02 03>",
        }));
        WITH_CONTEXT(requireError("must not be a multiple of 4"));
    }

    void testValueListLength() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 3", vr::RuleType::ValueList));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 10, 20, 30",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 10, 20",
        }));
        WITH_CONTEXT(requireError("The number of values in this list must be a multiple of 3"));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 3", vr::RuleType::ValueList));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 10, 20",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 10, 20, 30",
        }));
        WITH_CONTEXT(requireError("must not be a multiple of 3"));
    }

    void testValueMatrixRowsAndColumns() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 2, 3", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
            "    * 7, 8, 9",
        }));
        WITH_CONTEXT(requireError("The number of rows must be a multiple of 2"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2",
            "    * 4, 5",
        }));
        WITH_CONTEXT(requireError("The number of columns must be a multiple of 3"));

        WITH_CONTEXT(requireOneConstraintPass("not multiple: 2, 3", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x:",
            "    * 1, 2",
            "    * 4, 5",
            "    * 7, 8",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
        }));
        WITH_CONTEXT(requireError("must not be a multiple of 2"));
    }

    void testSectionEntryCountWithVrAny() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 2", vr::RuleType::Section));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "a: 1",
            "b: 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "a: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be a multiple of 2"));
    }

    void testSectionWithTextsEntryCountWithVrAny() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 2", vr::RuleType::SectionWithTexts));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "\"a\": 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be a multiple of 2"));
    }

    void testSectionListEntryCount() {
        WITH_CONTEXT(requireOneConstraintPass("multiple: 2", vr::RuleType::SectionList));
        WITH_CONTEXT(requirePassLines({
            "*[app.x]*",
            "y: 1",
            "*[app.x]*",
            "y: 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[app.x]*",
            "y: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section list must be a multiple of 2"));
    }
};
