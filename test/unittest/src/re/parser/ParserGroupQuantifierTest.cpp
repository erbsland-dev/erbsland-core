// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

using impl::limits::maximumQuantifierCount;

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserGroupQuantifierTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testSimpleQuantifiers_OnCapturingGroup() {
        // (a)? 0 or 1
        WITH_CONTEXT(parseAndTest(
            "(a)?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        // (a)?+ possessive, (a)?? lazy
        WITH_CONTEXT(parseAndTest(
            "(a)?+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=possessive,atomicGroupId=0)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a)??"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=lazy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        // * 0 or more
        WITH_CONTEXT(parseAndTest(
            "(a)*"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a)*+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=possessive,atomicGroupId=0)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a)*?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=lazy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        // + 1 or more
        WITH_CONTEXT(parseAndTest(
            "(a)+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a)++"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a)+?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=lazy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
    }

    void testCountedQuantifiers_OnGroupVariants() {
        // Capturing group
        WITH_CONTEXT(parseAndTest(
            "(a){3}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=3,max=3,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        WITH_CONTEXT(parseAndTest(
            "(a){2,5}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=5,mode=possessive,atomicGroupId=0)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        WITH_CONTEXT(parseAndTest(
            "(a){2,5}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=5,mode=lazy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        WITH_CONTEXT(parseAndTest(
            "(a){4,}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=4,max=*,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        WITH_CONTEXT(parseAndTest(
            "(a){,3}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=3,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        // Non-capturing group
        WITH_CONTEXT(parseAndTest(
            "(?:a){2,4}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=4,mode=possessive,atomicGroupId=0)",
                "      Group(size=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));

        // Atomic group
        WITH_CONTEXT(parseAndTest(
            "(?>a){3}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=3,max=3,mode=lazy)",
                "      Group(size=1,atomicGroupId=0,atomic)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            }));
    }

    void testExactOneOptimization_OnGroup() {
        // {1} and {1,1} must not create a quantifier node on groups as well
        WITH_CONTEXT(parseAndTest(
            "(a){1}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "(a){1,1}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
            }));
    }

    void testErrorCases_OnGroupQuantifier() {
        // max < min
        parser = Parser{"(a){5,3}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // missing closing '}'
        parser = Parser{"(a){2,3"_el};
        REQUIRE_THROWS(node = parser.parse());

        // invalid char after '{' (neither digit nor ',')
        parser = Parser{"(a){}}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // invalid after digits (neither ',' nor '}')
        parser = Parser{"(a){12x}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // too many digits
        parser = Parser{"(a){123456}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // too many digits
        parser = Parser{"(a){,123456}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // too many digits
        parser = Parser{"(a){1,123456}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // {,} requires digit after comma
        parser = Parser{"(a){,}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // {2,] => expected digit or '}' after ','
        parser = Parser{"(a){2,]}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testCustomSettings_Limits_OnGroupQuantifier() {
        Settings settings;
        settings.setMaximumQuantifierCount(3);

        // boundary success
        WITH_CONTEXT(parseAndTest(
            "(a){3,}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=3,max=*,mode=greedy)",
                "      Group(size=1,index=1)",
                "        Sequence(size=1)",
                "          CharacterSequence(\"a\")",
            },
            {},
            settings));

        // min exceeds
        parser = Parser{"(a){4,}"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());

        // max exceeds
        parser = Parser{"(a){1,4}"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }
};
