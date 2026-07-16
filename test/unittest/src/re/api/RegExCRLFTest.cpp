// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Newlines)
class RegExCRLFTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void testFoldingMatch() {
        // Test that \r\n is matched by \n in the pattern when Flag::CRLF is set.
        WITH_CONTEXT(requireCompile("\n"_el, Flags{Flag::CRLF}));

        WITH_CONTEXT(requireMatch("\r\n"_el));
        // Range should be 0-2 because \r\n is 2 bytes.
        // Even though it is folded to \n, the original bytes are preserved in capture groups.
        // RegExBase::createGroupLines uses toSafeString, which escapes \r and \n.
        const auto expectedGroups = std::vector<std::string>{
            "00: 0000-0002 '\\r\\n'",
        };
        WITH_CONTEXT(requireGroups(expectedGroups));
    }

    void testNoFoldingWithoutFlag() {
        // Test that \r\n is NOT matched by \n in the pattern when Flag::CRLF is NOT set.
        WITH_CONTEXT(requireCompile("\n"_el, Flags{}));

        WITH_CONTEXT(requireNoMatch("\r\n"_el));
    }

    void testPreserveLoneCR() {
        // Test that a lone \r is NOT folded.
        WITH_CONTEXT(requireCompile("\r"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireMatch("\r"_el));

        const auto expectedGroups = std::vector<std::string>{
            "00: 0000-0001 '\\r'",
        };
        WITH_CONTEXT(requireGroups(expectedGroups));

        WITH_CONTEXT(requireCompile("\n"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireNoMatch("\r"_el));
    }

    void testMultipleFolding() {
        WITH_CONTEXT(requireCompile("a\nb\nc"_el, Flags{Flag::CRLF}));

        WITH_CONTEXT(requireMatch("a\r\nb\nc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0006 'a\\r\\nb\\nc'",
        }));

        WITH_CONTEXT(requireMatch("a\nb\r\nc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0006 'a\\nb\\r\\nc'",
        }));

        WITH_CONTEXT(requireMatch("a\r\nb\r\nc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0007 'a\\r\\nb\\r\\nc'",
        }));
    }

    void testDotMatchesCRLF() {
        // When CRLF is folded to \n, the dot operator behavior depends on DotAll.
        // By default, dot does not match \n.
        WITH_CONTEXT(requireCompile("."_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireNoMatch("\r\n"_el));

        // With DotAll, dot matches \n, so it should match the folded \r\n.
        WITH_CONTEXT(requireCompile("."_el, Flags{Flag::CRLF, Flag::DotAll}));
        WITH_CONTEXT(requireMatch("\r\n"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 '\\r\\n'",
        }));
    }

    void testFoldingInCaptureGroups() {
        // Test capture without folding
        WITH_CONTEXT(requireCompile("(a)"_el, Flags{}));
        WITH_CONTEXT(requireMatch("a"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
        }));

        // Very simple capture with folding
        WITH_CONTEXT(requireCompile("(\n)"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireMatch("\r\n"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 '\\r\\n'",
            "01: 0000-0002 '\\r\\n'",
        }));

        // Test with single group around the folded character
        WITH_CONTEXT(requireCompile("a(\n)b"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireMatch("a\r\nb"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'a\\r\\nb'",
            "01: 0001-0003 '\\r\\n'",
        }));

        // Test with multiple groups
        WITH_CONTEXT(requireCompile("(a)\n(b)"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireMatch("a\r\nb"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'a\\r\\nb'",
            "01: 0000-0001 'a'",
            "02: 0003-0004 'b'",
        }));
    }

    void testAnchorsWithCRLF() {
        // Multiline mode: ^ and $ match around \n.
        // With CRLF folding, they should match around \r\n.
        WITH_CONTEXT(requireCompile("^abc$"_el, Flags{Flag::Multiline, Flag::CRLF}));

        WITH_CONTEXT(requireMatch("abc"_el));
        WITH_CONTEXT(requireFindFirst("\r\nabc\r\n"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0002-0005 'abc'",
        }));
    }

    void testLiteralCRLF() {
        WITH_CONTEXT(requireCompile("\r\n"_el, Flags{Flag::CRLF}));
        WITH_CONTEXT(requireNoMatch("\r\n"_el));
    }
};
