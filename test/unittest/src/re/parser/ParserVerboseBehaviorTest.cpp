// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserVerboseBehaviorTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testNonVerbose_HashAndSpaceAreLiterals() {
        // Single '#'
        WITH_CONTEXT(parseAndTest(
            "#"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"#\")",
            }));

        // Single space
        WITH_CONTEXT(parseAndTest(
            " "_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\" \")",
            }));

        // Mixed: spaces and '#'
        WITH_CONTEXT(parseAndTest(
            "a  #  b"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a  #  b\")",
            }));
    }

    void testVerbose_WhitespaceIgnoredAndHashStartsComment() {
        // With verbose via start flags, spaces are ignored and '#' starts a comment until end of line
        WITH_CONTEXT(parseAndTest(
            "(?x)  a  # comment\n  b  "_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            }));
    }

    void testVerbose_EscapedSpaceAndHashAreLiterals() {
        // In verbose mode, an escaped space ("\\ ") and escaped hash ("\\#") are treated as literals
        // Result should be "a # b"
        WITH_CONTEXT(parseAndTest(
            "(?x)a\\ \\#\\ b"_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a # b\")",
            }));

        // Only escaped space and hash
        WITH_CONTEXT(parseAndTest(
            "(?x)\\ \\#"_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterSequence(\" #\")",
            }));
    }

    void testVerbose_InsideCharacterClass_NoCommentNoWhitespaceSkip() {
        // Inside character classes, verbose mode does not treat '#' as comment and does not skip spaces
        WITH_CONTEXT(parseAndTest(
            "(?x)[#]"_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterClass([#])",
            }));

        WITH_CONTEXT(parseAndTest(
            "(?x)[ ]"_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterClass([\\u0020])",
            }));
    }
};
