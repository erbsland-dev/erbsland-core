// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing the 'equals' constraint.
TESTED_TARGETS(EqualsConstraint)
TAGS(ValidationRules)
class VrEqualsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // Test the combination of the constraint with the rule type.
        // The `equals` constraint is particularly complex, as it allows doing an equal comparison with a
        // range of values, but when used with an integer, it compares the element count in lists or
        // the character/byte count in text/bytes.

        // test 'equals' using an integer value.
        WITH_CONTEXT(requireConstraintValidForRuleTypes(
            "equals: 3",
            {vr::RuleType::Integer,
                vr::RuleType::Text,
                vr::RuleType::Bytes,
                vr::RuleType::ValueList,
                vr::RuleType::Section,
                vr::RuleType::SectionList,
                vr::RuleType::SectionWithTexts}));
        // test 'equals' using a boolean value.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("equals: true", {vr::RuleType::Boolean}));
        // test 'equals' using a float value.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("equals: 2.0", {vr::RuleType::Float}));
        // test 'equals' using a text value.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("equals: \"text\"", {vr::RuleType::Text}));
        // test 'equals' using a byte data value.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("equals: <01 02>", {vr::RuleType::Bytes}));
        // test 'equals' using two integers.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("equals: 3, 3", {vr::RuleType::ValueMatrix}));
    }

    void testUnsupportedTypes() {
        // Test the constraint with unsupported values.
        WITH_CONTEXT(requireOneConstraintFail("equals: 2026-01-01", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
        WITH_CONTEXT(requireOneConstraintFail("equals: 12:00:02", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
        WITH_CONTEXT(requireOneConstraintFail("equals: 2026-01-01 12:00:02", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
        WITH_CONTEXT(requireOneConstraintFail("equals: 1, 2, 3", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
        WITH_CONTEXT(requireOneConstraintFail("equals: 10 minutes", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
        WITH_CONTEXT(requireOneConstraintFail("equals: /abc/", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'equals' constraint for a text rule must be a text or integer"));
    }

    void testEqualsWithInteger() {
        // Test the actual constraint logic with an integer value.
        WITH_CONTEXT(requireOneConstraintPass("equals: 42", vr::RuleType::Integer));
        // Always test pass and fail cases to ensure failures are caused by the constraint logic and
        // not by a side effect.
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 42",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 43",
        }));
        WITH_CONTEXT(requireError("The value must be equal to 42"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0",
        }));
        WITH_CONTEXT(requireError("The value must be equal to 42"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: -2'003'928",
        }));
        WITH_CONTEXT(requireError("The value must be equal to 42"));
    }

    void testNotEqualsWithInteger() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 42", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 100",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 42",
        }));
        WITH_CONTEXT(requireError("The value must not be equal to 42"));
    }

    void testEqualsWithBoolean() {
        WITH_CONTEXT(requireOneConstraintPass("equals: true", vr::RuleType::Boolean));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: Yes",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: No",
        }));
        WITH_CONTEXT(requireError("The value must be true"));
    }

    void testNotEqualsWithBoolean() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: true", vr::RuleType::Boolean));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: Disabled",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: Enabled",
        }));
        WITH_CONTEXT(requireError("The value must be false"));
    }

    void testEqualsWithFloat() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 0.3", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.3",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.2999",
        }));
        WITH_CONTEXT(requireError("The value must be equal to 0.3"));

        WITH_CONTEXT(requireOneConstraintPass("equals: NaN", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: NaN",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.0",
        }));
        WITH_CONTEXT(requireError("The value must be equal to nan"));

        WITH_CONTEXT(requireOneConstraintPass("equals: inf", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: -inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.0",
        }));
        WITH_CONTEXT(requireError("The value must be equal to inf"));

        WITH_CONTEXT(requireOneConstraintPass("equals: -inf", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.0",
        }));
        WITH_CONTEXT(requireError("The value must be equal to -inf"));
    }

    void testNotEqualsWithFloat() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 0.3", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.2999",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.3",
        }));
        WITH_CONTEXT(requireError("The value must not be equal to 0.3"));

        WITH_CONTEXT(requireOneConstraintPass("not_equals: NaN", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.0",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: NaN",
        }));
        WITH_CONTEXT(requireError("The value must not be equal to nan"));

        WITH_CONTEXT(requireOneConstraintPass("not_equals: inf", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: -inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: inf",
        }));
        WITH_CONTEXT(requireError("The value must not be equal to inf"));

        WITH_CONTEXT(requireOneConstraintPass("not_equals: -inf", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: -inf",
        }));
        WITH_CONTEXT(requireError("The value must not be equal to -inf"));
    }

    void testEqualsWithText() {
        // text comparison is case-insensitive by default.
        WITH_CONTEXT(requireOneConstraintPass("equals: \"abc\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"ABC\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"xyz\"",
        }));
        WITH_CONTEXT(requireError("The text must be equal to \"abc\" (case-insensitive)"));
    }

    void testEqualsWithTextCaseSensitive() {
        // Test the case-sensitive comparison
        WITH_CONTEXT(requireOneConstraintPass("equals: \"abc\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"ABC\"",
        }));
        WITH_CONTEXT(requireError("The text must be equal to \"abc\" (case-sensitive)"));
    }

    void testNotEqualsWithText() {
        // text comparison is case-insensitive by default.
        WITH_CONTEXT(requireOneConstraintPass("not_equals: \"abc\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"xyz\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requireError("The text must not be equal to \"abc\" (case-insensitive)"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"ABC\"",
        }));
        WITH_CONTEXT(requireError("The text must not be equal to \"abc\" (case-insensitive)"));
    }

    void testEqualsWithBytes() {
        WITH_CONTEXT(requireOneConstraintPass("equals: <01 02 03>", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <010203>",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <<<",
            "    01 02 03",
            "    >>>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <01 02 04>",
        }));
        WITH_CONTEXT(requireError("The byte sequence must be equal to \"010203\""));
    }

    void testEqualsWithLongBytesDiagnostic() {
        WITH_CONTEXT(requireOneConstraintPass(
            "equals: <00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13>", vr::RuleType::Bytes));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <ff>",
        }));
        REQUIRE(lastError.contains("The byte sequence must be equal to \"0001020304050607…0d0e0f10111213\""_el));
    }

    void testEqualsWithCharacterCount() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::Text));
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
            "x: \"\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be equal to 3"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"hello\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be equal to 3"));
    }

    void testNotEqualsWithCharacterCount() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"hello\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must not be equal to 3"));
    }

    void testEqualsWithByteCount() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <010203>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <01020304>",
        }));
        WITH_CONTEXT(requireError("The number of bytes must be equal to 3"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: < >",
        }));
        WITH_CONTEXT(requireError("The number of bytes must be equal to 3"));
    }

    void testNotEqualsWithByteCount() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <01 02 03 04>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <01 02 03>",
        }));
        WITH_CONTEXT(requireError("The number of bytes must not be equal to 3"));
    }

    void testElementCountInValueList() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::ValueList));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 1, 2, 3",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 1, 2",
        }));
        WITH_CONTEXT(requireError("The number of values in this list must be equal to 3"));
    }

    void testNotElementCountInValueList() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::ValueList));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 1, 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 1, 2, 3",
        }));
        WITH_CONTEXT(requireError("The number of values in this list must not be equal to 3"));
    }

    void testElementCountInValueMatrix() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3, 3", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
            "    * 7, 8, 9",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 1, 2, 3",
        }));
        WITH_CONTEXT(requireError("The number of columns must be equal to 3"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6, 0",
            "    * 7, 8, 9",
        }));
        WITH_CONTEXT(requireError("The number of columns must be equal to 3"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
            "    * 7, 8, 9",
            "    * 10, 11, 12",
        }));
        WITH_CONTEXT(requireError("The number of rows must be equal to 3"));
    }

    void testNotElementCountInValueMatrix() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3, 3", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 1, 2",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
            "    * 7, 8, 9",
        }));
        WITH_CONTEXT(requireError("The number of rows must not be equal to 3"));
        // fails if rows *or* columns are equal to 3, as both are tested independently.
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x:",
            "    * 1, 2",
            "    * 4, 5",
            "    * 7, 8",
        }));
    }

    void testSectionCountInSectionList() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::SectionList));
        WITH_CONTEXT(requirePassLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section list must be equal to 3"));
    }

    void testNotSectionCountInSectionList() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::SectionList));
        WITH_CONTEXT(requirePassLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
            "*[app.x]",
            "y: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section list must not be equal to 3"));
    }

    void testEntryCountInSection() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::Section));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "a: 1",
            "b: 1",
            "c: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "a: 1",
            "b: 1",
            "c: 1",
            "d: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be equal to 3"));
    }

    void testNotEntryCountInSection() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::Section));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "a: 1",
            "b: 1",
            "c: 1",
            "d: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "a: 1",
            "b: 1",
            "c: 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must not be equal to 3"));
    }

    void testEntryCountInSectionWithTexts() {
        WITH_CONTEXT(requireOneConstraintPass("equals: 3", vr::RuleType::SectionWithTexts));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 1",
            "\"c\": 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 1",
            "\"c\": 1",
            "\"d\": 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be equal to 3"));
    }

    void testNotEntryCountInSectionWithTexts() {
        WITH_CONTEXT(requireOneConstraintPass("not_equals: 3", vr::RuleType::SectionWithTexts));
        WITH_CONTEXT(requirePassLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 1",
            "\"c\": 1",
            "\"d\": 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app.x]",
            "\"a\": 1",
            "\"b\": 1",
            "\"c\": 1",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must not be equal to 3"));
    }
};
