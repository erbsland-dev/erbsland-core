// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;
using namespace el::text::literals;

// Testing the 'chars' and 'not_chars' constraint.
TESTED_TARGETS(CharsConstraint)
TAGS(ValidationRules)
class VrCharsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        WITH_CONTEXT(requireConstraintValidForRuleTypes("chars: \"(a-z)\"", {vr::RuleType::Text}));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("not_chars: \"(a-z)\"", {vr::RuleType::Text}));
    }

    void testValueMustBeTextOrTextList() {
        WITH_CONTEXT(requireOneConstraintFail("chars: 42", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'chars' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("chars: 42, 43", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'chars' constraint must specify a single text value or a list of texts"));

        WITH_CONTEXT(requireOneConstraintFail("not_chars: true", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The 'not_chars' constraint must specify a single text value or a list of texts"));
    }

    void testCharsUnionSemantics() {
        WITH_CONTEXT(requireOneConstraintPass("chars: \"(a-z)\", \"[%]\", \"digits\"", vr::RuleType::Text));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"abc\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"501%\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"abc_501\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 3: \"_\""));
    }

    void testCharsIsAlwaysCaseSensitive() {
        WITH_CONTEXT(requireOneConstraintPass("chars: \"(a-z)\"", vr::RuleType::Text));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"A\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0: \"A\""));

        WITH_CONTEXT(requireOneConstraintPass("chars: \"(a-z)\"", vr::RuleType::Text, true));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"A\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0: \"A\""));
    }

    void testNotCharsSemantics() {
        WITH_CONTEXT(requireOneConstraintPass("not_chars: \"(a-z)\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"12345\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"123abc\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 3: \"a\""));
    }

    void testNamedRanges() {
        WITH_CONTEXT(requireOneConstraintPass("chars: \"letters\", \"digits\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"Abc012\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"Abc_012\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 3: \"_\""));

        WITH_CONTEXT(requireOneConstraintPass("chars: \"spacing\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            R"(x: "\t \t")",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            R"(x: "_")",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0: \"_\""));

        WITH_CONTEXT(requireOneConstraintPass("chars: \"linebreak\"", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            R"(x: "\n\r\n")",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            R"(x: " ")",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0: \" \""));
    }

    void testRangeSyntaxValidation() {
        WITH_CONTEXT(requireOneConstraintFail("chars: \"(ab)\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("Invalid character range syntax"));

        WITH_CONTEXT(requireOneConstraintFail("chars: \"(a_z)\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("Invalid character range syntax"));

        WITH_CONTEXT(requireOneConstraintFail("chars: \"(z-a)\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("start (U+007A) must be lower than end (U+0061)"));

        WITH_CONTEXT(requireOneConstraintFail("chars: \"(a-a)\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("start (U+0061) must be lower than end (U+0061)"));

        // Spec: ranges with combining characters are invalid.
        WITH_CONTEXT(requireOneConstraintFail("chars: \"(\\u{301}-z)\"", vr::RuleType::Text));
    }

    void testCharacterListValidation() {
        WITH_CONTEXT(requireOneConstraintFail("chars: \"[aab]\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("The character list contains a duplicate character: 'a'"));
    }

    void testNamedRangeValidation() {
        WITH_CONTEXT(requireOneConstraintFail("chars: \"unicode_letters\"", vr::RuleType::Text));
        WITH_CONTEXT(requireError("Unknown named character range: \"unicode_letters\""));
    }

    void testCharsAndNotCharsCannotBeCombined() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"text\"",
            "chars: \"(a-z)\"",
            "not_chars: \"digits\"",
        }));
        WITH_CONTEXT(requireError("You must not mix positive and negative constraints for the same type"));
    }

    void testValidationPositionCountsCodePoints() {
        WITH_CONTEXT(requireOneConstraintPass("chars: \"[😀a]\"", vr::RuleType::Text));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"😀_\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 1: \"_\""));
    }

    void testSecretValuesHideForbiddenCharacter() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "is_secret: yes",
            "chars: \"(a-z)\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"A\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0 in a secret value"));
        REQUIRE_FALSE(lastError.contains("position 0: \""_el, el::text::cCaseInsensitive.asciiComparisonFn()));
        REQUIRE_FALSE(lastError.contains("\"A\""_el, el::text::cCaseInsensitive.asciiComparisonFn()));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"text\"",
            "is_secret: yes",
            "not_chars: \"(a-z)\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"a\"",
        }));
        WITH_CONTEXT(requireError("The text contains a forbidden character at position 0 in a secret value"));
        REQUIRE_FALSE(lastError.contains("position 0: \""_el, el::text::cCaseInsensitive.asciiComparisonFn()));
        REQUIRE_FALSE(lastError.contains("\"a\""_el, el::text::cCaseInsensitive.asciiComparisonFn()));
    }
};
