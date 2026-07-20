// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyExactQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharExact() { WITH_CONTEXT(requireCompile("a{3}"_el)); }

    void compilePrefixSingleCharExact() { WITH_CONTEXT(requireCompile("xa{2}"_el)); }

    void compilePrefixSingleCharExactPostfix() { WITH_CONTEXT(requireCompile("xa{2}y"_el)); }

    void compilePrefixGroupExactPostfix() { WITH_CONTEXT(requireCompile("x(?:xxx){2}y"_el)); }

    void compileNestedGroupsExact() { WITH_CONTEXT(requireCompile("x(?:a{2}b{2}(?:xyz){2}){2}y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharExact());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaa"_el},
                {"aaab"_el, 0, "aaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "aa"_el,
                "baa"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharExact());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaa"_el},
                {"xaaaa"_el, 0, "xaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "xa"_el,
                "ya"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaay"_el, 0, "xaay"_el},
                {"xaayx"_el, 0, "xaay"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "xay"_el,
                "xaa"_el,
                "yaay"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupExactPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xxxxxxxy"_el, 0, "xxxxxxxy"_el},
                {"xxxxxxxyzzz"_el, 0, "xxxxxxxy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "xxxxy"_el,
                "x"_el,
                "xxxy"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsExact());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaabbxyzxyzaabbxyzxyzy"_el, 0, "xaabbxyzxyzaabbxyzxyzy"_el},
                {"xaabbxyzxyzaabbxyzxyzyx"_el, 0, "xaabbxyzxyzaabbxyzxyzy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xaabbxyzxyzy"_el,
                "xxyzxyz"_el,
                "x"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharExact());
        {
            const auto matchCases = std::vector<String>{
                "aaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "aa"_el,
                "aaaa"_el,
                "baaa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());
        {
            const auto matchCases = std::vector<String>{
                "xaay"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xay"_el,
                "xaayx"_el,
                "xaayy"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsExact());
        {
            const auto matchCases = std::vector<String>{
                "xaabbxyzxyzaabbxyzxyzy"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xaabbxyzxyzaabbxyzxyzyx"_el,
                "xaabbxyzxyzy"_el,
                "x"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaay"_el, 2, "xaay"_el},
            {"xaay"_el, 0, "xaay"_el},
            {"xaayxaay"_el, 0, "xaay"_el},
            {"😀xaay"_el, 4, "xaay"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "xay"_el,
            "xaaa"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaay'",
            "Match 02:",
            "00: 0005-0009 'xaay'",
            "Match 03:",
            "00: 0010-0014 'xaay'",
        };
        WITH_CONTEXT(requireFindAll("xaay xaay xaay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaay xaay xaay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "xay"_el,
            "xa"_el,
            "y"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaay'",
            "Match 02:",
            "00: 0005-0009 'xaay'",
            "Match 03:",
            "00: 0010-0014 'xaay'",
        };
        WITH_CONTEXT(requireCollectAll("xaay xaay xaay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaay xaay xaay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharExactPostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"xaay"_el, ""_el, ""_el},
            {"xaay"_el, "<x>"_el, "<x>"_el},
            {"xaay xaay"_el, "<x>"_el, "<x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testExactQuantifierModifiers() {
        {
            WITH_CONTEXT(requireCompile("xa{2}?y"_el));
            const auto matchCases = NoneCapGroupTestCases{
                {"xaay"_el, 0, "xaay"_el},
                {"xaayx"_el, 0, "xaay"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xay"_el,
                "xa"_el,
                "y"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        {
            WITH_CONTEXT(requireCompile("xa{2}+y"_el));
            const auto matchCases = NoneCapGroupTestCases{
                {"xaay"_el, 0, "xaay"_el},
                {"xaayx"_el, 0, "xaay"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xay"_el,
                "xa"_el,
                "y"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    void testSingleRepetitionFolding() {
        WITH_CONTEXT(requireCompile("x{1}"_el));
        const auto foldedLines = diagnostics::Disassembler{regex}.disassemble();

        WITH_CONTEXT(requireCompile("x"_el));
        const auto directLines = diagnostics::Disassembler{regex}.disassemble();

        REQUIRE_EQUAL(foldedLines, directLines);
    }
};
