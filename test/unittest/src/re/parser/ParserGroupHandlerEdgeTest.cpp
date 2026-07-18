// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/impl/Limits.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserGroupHandlerEdgeTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testGroupFlags_MinusClearsImsx() {
        // Ensure we pass through the minus-branch for i/m/s/x and clear them.
        // Pattern creates a non-capturing group with flags section and simple content.
        // We don't assert flags directly (not exposed), but successful parse covers the code paths.
        REQUIRE_NOTHROW(Parser{"(?imsx-imsx:a)"_el}.parse());
        // Also allow a pattern-start only flags set without minus (sanity) – should succeed
        REQUIRE_NOTHROW(Parser{"(?imsx)a"_el}.parse());
    }

    void testGroupComment_EscapedClosingParen() {
        // Inside an inline comment, an escaped ')' must be consumed and not terminate the comment.
        // Place the comment inside a non-capturing group so the following ')' has a group to close.
        REQUIRE_NOTHROW(Parser{"(?:(?#abc\\)def)x)"_el}.parse());
        REQUIRE_NOTHROW(Parser{"(?:(?#\\))x)"_el}.parse());
    }

    void testNamedGroup_NameTooLong() {
        // Construct a name longer than limits::maximumGroupNameLength
        constexpr auto maxLen = impl::limits::maximumGroupNameLength;
        auto patternU8 = StringEditor{"(?<"_el};
        patternU8.append(el::text::Char{U'a'}, el::unit::CpLength{maxLen + 2U});
        patternU8.append(">x)"_el);
        Parser p{String{patternU8}};
        REQUIRE_THROWS(node = p.parse());
    }

    void testAdvancedGroups_UnsupportedVariants() {
        // (?R) Recursive groups are not supported
        parser = Parser{"(?R)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Subpattern calls are not supported: digits, '+', '&'
        parser = Parser{"(?1)"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"(?+)"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"(?&a)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Conditional patterns are not supported: "?(" right after the advanced opener
        parser = Parser{"(?()"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testUnexpectedGroupFlag_DefaultBranch() {
        // Unknown flag letter triggers the default branch (throws internal error) while scanning flags
        parser = Parser{"(?q:a)"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Also at pattern start variant
        parser = Parser{"(?q)"_el};
        REQUIRE_THROWS(node = parser.parse());
    }
};
