// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/impl/Limits.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserCharacterSequenceTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testCharacterSequence() {
        const auto testCases = TestCases{
            {// simple ascii
                "abc"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=1)",
                    "    CharacterSequence(\"abc\")",
                }},
            {// unicode
                "→😆●xyz"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=1)",
                    "    CharacterSequence(\"→😆●xyz\")",
                }},
        };
        WITH_CONTEXT(requireTestCases(testCases));
    }

    void testRepeatedCharacters() {
        const auto testCases = TestCases{
            {// first test a combined sequence to see if this is working.
                "abc+def"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=3)",
                    "    CharacterSequence(\"ab\")",
                    "    Quantifier(min=1,max=*,mode=greedy)",
                    "      CharacterSequence(\"c\")",
                    "    CharacterSequence(\"def\")",
                }},
            {// first test a combined sequence to see if this is working.
                "ax?bx??cx*?dx++e"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=9)",
                    "    CharacterSequence(\"a\")",
                    "    Quantifier(min=0,max=1,mode=greedy)",
                    "      CharacterSequence(\"x\")",
                    "    CharacterSequence(\"b\")",
                    "    Quantifier(min=0,max=1,mode=lazy)",
                    "      CharacterSequence(\"x\")",
                    "    CharacterSequence(\"c\")",
                    "    Quantifier(min=0,max=*,mode=lazy)",
                    "      CharacterSequence(\"x\")",
                    "    CharacterSequence(\"d\")",
                    "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                    "      CharacterSequence(\"x\")",
                    "    CharacterSequence(\"e\")",
                }},
            {"x?y??z?+"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=3)",
                    "    Quantifier(min=0,max=1,mode=greedy)",
                    "      CharacterSequence(\"x\")",
                    "    Quantifier(min=0,max=1,mode=lazy)",
                    "      CharacterSequence(\"y\")",
                    "    Quantifier(min=0,max=1,mode=possessive,atomicGroupId=0)",
                    "      CharacterSequence(\"z\")",
                }},
            {"x+y+?z++"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=3)",
                    "    Quantifier(min=1,max=*,mode=greedy)",
                    "      CharacterSequence(\"x\")",
                    "    Quantifier(min=1,max=*,mode=lazy)",
                    "      CharacterSequence(\"y\")",
                    "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                    "      CharacterSequence(\"z\")",
                }},
            {"x*y*?z*+"_el,
                {
                    "Group(size=1)",
                    "  Sequence(size=3)",
                    "    Quantifier(min=0,max=*,mode=greedy)",
                    "      CharacterSequence(\"x\")",
                    "    Quantifier(min=0,max=*,mode=lazy)",
                    "      CharacterSequence(\"y\")",
                    "    Quantifier(min=0,max=*,mode=possessive,atomicGroupId=0)",
                    "      CharacterSequence(\"z\")",
                }},
        };
        WITH_CONTEXT(requireTestCases(testCases));
    }

    void testCharacterSequenceMaximumLength() {
        const auto pattern = String::fromCharacter(
            el::text::Char{U'a'}, el::unit::CpLength{impl::limits::maximumCharacterSequenceLength + 1U});
        parser = Parser{pattern};
        REQUIRE_THROWS(node = parser.parse());
    }
};
