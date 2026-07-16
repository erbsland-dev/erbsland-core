// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserAlternativeLimitsTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testCustomAlternativeLimitAtRoot() {
        Settings settings;
        settings.setMaximumAlternativeCount(2);

        // Exactly two alternatives at root -> OK
        parser = Parser{"a|b"_el, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());

        // Three alternatives at root -> exceeds custom limit -> must throw
        parser = Parser{"a|b|c"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testCustomAlternativeLimitInGroup() {
        Settings settings;
        settings.setMaximumAlternativeCount(1);

        // One alternative inside a group already exceeds the limit of 1
        parser = Parser{"(a|b)"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());

        // No alternative inside a group is fine
        parser = Parser{"(ab)"_el, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
    }
};
