// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing the 'in' constraint.
TESTED_TARGETS(InConstraint)
TAGS(ValidationRules)
class VrInTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        WITH_CONTEXT(requireConstraintValidForRuleTypes("in: 3", {vr::RuleType::Integer}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("in: 2.0", {vr::RuleType::Float}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("in: \"value\"", {vr::RuleType::Text}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("in: <01 02>", {vr::RuleType::Bytes}));
    }

    void testTypeAlignmentInDefinition() {
        WITH_CONTEXT(requireOneConstraintFail("in: \"42\"", vr::RuleType::Integer));
        WITH_CONTEXT(
            requireError("The 'in' constraint must specify a single integer value or a list of integer values"));

        WITH_CONTEXT(requireOneConstraintFail("in: 1", vr::RuleType::Float));
        WITH_CONTEXT(requireError("The 'in' constraint must specify a single float value or a list of float values"));

        WITH_CONTEXT(requireOneConstraintFail("in: 1", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'in' constraint must specify a single text value or a list of text values"));

        WITH_CONTEXT(requireOneConstraintFail("in: \"0102\"", vr::RuleType::Bytes));
        WITH_CONTEXT(requireError("The 'in' constraint must specify a single bytes value or a list of bytes values"));

        WITH_CONTEXT(requireOneConstraintFail("in: 1, \"2\"", vr::RuleType::Integer));
        WITH_CONTEXT(
            requireError("The 'in' constraint must specify a single integer value or a list of integer values"));
    }

    void testNoDuplicateEntriesInDefinition() {
        WITH_CONTEXT(requireOneConstraintFail("in: 3, 3", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'in' list must not contain duplicate values"));

        WITH_CONTEXT(requireOneConstraintFail("in: 0.3, 0.30000000000000004", vr::RuleType::Float));
        WITH_CONTEXT(requireError("The 'in' list must not contain duplicate values"));

        WITH_CONTEXT(requireOneConstraintFail("in: \"alpha\", \"ALPHA\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'in' list must not contain duplicate values"));

        WITH_CONTEXT(requireOneConstraintPass("in: \"alpha\", \"ALPHA\"", vr::RuleType::Text, true));

        WITH_CONTEXT(requireOneConstraintFail("in: <12 34>, <12 34>", vr::RuleType::Bytes));
        WITH_CONTEXT(requireError("The 'in' list must not contain duplicate values"));
    }

    void testInWithInteger() {
        WITH_CONTEXT(requireOneConstraintPass("in: 10, 20, 30", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 20",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 9",
        }));
        WITH_CONTEXT(requireError("The value must be one of 10 or 20 or 30"));
    }

    void testNotInWithInteger() {
        WITH_CONTEXT(requireOneConstraintPass("not_in: 10, 20, 30", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 9",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 20",
        }));
        WITH_CONTEXT(requireError("The value must not be one of 10 or 20 or 30"));
    }

    void testInWithFloat() {
        WITH_CONTEXT(requireOneConstraintPass("in: 0.3, 1.0", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.3",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.30000000000000004",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.2999",
        }));
        WITH_CONTEXT(requireError("The value must be one of"));
        WITH_CONTEXT(requireError("0.3"));
        WITH_CONTEXT(requireError("1"));
        WITH_CONTEXT(requireError("within platform tolerance"));
    }

    void testNotInWithFloat() {
        WITH_CONTEXT(requireOneConstraintPass("not_in: 0.3, 1.0", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 2.0",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.3",
        }));
        WITH_CONTEXT(requireError("The value must not be one of"));
        WITH_CONTEXT(requireError("0.3"));
        WITH_CONTEXT(requireError("1"));
        WITH_CONTEXT(requireError("within platform tolerance"));
    }

    void testInWithTextCaseInsensitive() {
        WITH_CONTEXT(requireOneConstraintPass("in: \"idle\", \"scanning\", \"connecting\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"SCANNING\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"shutdown\"",
        }));
        WITH_CONTEXT(
            requireError("The text must be one of \"idle\" or \"scanning\" or \"connecting\" (case-insensitive)"));
    }

    void testInWithTextCaseSensitive() {
        WITH_CONTEXT(requireOneConstraintPass("in: \"idle\", \"scanning\", \"connecting\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"scanning\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"SCANNING\"",
        }));
        WITH_CONTEXT(
            requireError("The text must be one of \"idle\" or \"scanning\" or \"connecting\" (case-sensitive)"));
    }

    void testNotInWithText() {
        WITH_CONTEXT(requireOneConstraintPass("not_in: \"idle\", \"scanning\", \"connecting\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"shutdown\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"CONNECTING\"",
        }));
        WITH_CONTEXT(
            requireError("The text must not be one of \"idle\" or \"scanning\" or \"connecting\" (case-insensitive)"));
    }

    void testInWithBytes() {
        WITH_CONTEXT(requireOneConstraintPass("in: <01 02>, <ab cd ef>", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <0102>",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <ABCDEF>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <ff ff>",
        }));
        WITH_CONTEXT(requireError("The byte sequence must be one of"));
        WITH_CONTEXT(requireError("0102"));
        WITH_CONTEXT(requireError("ABCDEF"));
    }

    void testInWithLongBytesDiagnostic() {
        WITH_CONTEXT(requireOneConstraintPass(
            "in: <00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13>, <aa bb>", vr::RuleType::Bytes));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <ff>",
        }));
        REQUIRE(lastError.contains("\"0001020304050607…0d0e0f10111213\" or \"aabb\""_el));
    }

    void testNotInWithBytes() {
        WITH_CONTEXT(requireOneConstraintPass("not_in: <01 02>, <ab cd ef>", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <ff ff>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <ab cd ef>",
        }));
        WITH_CONTEXT(requireError("The byte sequence must not be one of"));
        WITH_CONTEXT(requireError("0102"));
        WITH_CONTEXT(requireError("ABCDEF"));
    }

    void testCustomErrorMessage() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "in: \"idle\", \"scanning\", \"connecting\"",
            "in_error: \"Choose one of the supported modes.\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"shutdown\"",
        }));
        WITH_CONTEXT(requireError("Choose one of the supported modes."));
    }
};
