// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserConstructorTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testConstructor() {
        Settings settings;
        settings.enableFeature(Feature::EmptyGroups);
        parser = Parser{String{}, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        REQUIRE(node->isGroup());
        REQUIRE_EQUAL(node->size(), 1U);
        REQUIRE(node->children()[0]->isSequence());
        parser = Parser{""_el, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        REQUIRE(node->isGroup());
        REQUIRE_EQUAL(node->size(), 1U);
        REQUIRE(node->children()[0]->isSequence());
        parser = Parser{StringEditor{}, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        REQUIRE(node->isGroup());
        REQUIRE_EQUAL(node->size(), 1U);
        REQUIRE(node->children()[0]->isSequence());
    }

    void testInitialAtomicFlagIsError() {
        // The atomic flag must not be allowed as an initial parser flag
        parser = Parser{"a"_el, GroupFlag::Atomic};
        REQUIRE_THROWS(node = parser.parse());
    }
};
