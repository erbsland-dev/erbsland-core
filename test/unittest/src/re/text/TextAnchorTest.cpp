// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/text/TextAnchor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::re;
using namespace el::text::literals;
using impl::TextAnchor;

TESTED_TARGETS(TextAnchor)
TAGS(Text Anchors)
class TextAnchorTest final : public el::UnitTest {
public:
    void testDefaultConstructionAndRaw() {
        // Default constructed TextAnchor uses the None value
        TextAnchor def{};
        REQUIRE_EQUAL(def.raw(), TextAnchor::None);

        // Construct from all defined enum values and verify raw()
        TextAnchor start{TextAnchor::Start};
        TextAnchor end{TextAnchor::End};
        TextAnchor lineStart{TextAnchor::LineStart};
        TextAnchor lineEnd{TextAnchor::LineEnd};
        TextAnchor unicodeWordBoundary{TextAnchor::UnicodeWordBoundary};
        TextAnchor asciiWordBoundary{TextAnchor::AsciiWordBoundary};
        TextAnchor nonUnicodeWordBoundary{TextAnchor::NonUnicodeWordBoundary};
        TextAnchor nonAsciiWordBoundary{TextAnchor::NonAsciiWordBoundary};

        REQUIRE_EQUAL(start.raw(), TextAnchor::Start);
        REQUIRE_EQUAL(end.raw(), TextAnchor::End);
        REQUIRE_EQUAL(lineStart.raw(), TextAnchor::LineStart);
        REQUIRE_EQUAL(lineEnd.raw(), TextAnchor::LineEnd);
        REQUIRE_EQUAL(unicodeWordBoundary.raw(), TextAnchor::UnicodeWordBoundary);
        REQUIRE_EQUAL(asciiWordBoundary.raw(), TextAnchor::AsciiWordBoundary);
        REQUIRE_EQUAL(nonUnicodeWordBoundary.raw(), TextAnchor::NonUnicodeWordBoundary);
        REQUIRE_EQUAL(nonAsciiWordBoundary.raw(), TextAnchor::NonAsciiWordBoundary);

        // Copy construction and assignment should preserve the value
        TextAnchor copy{start};
        REQUIRE_EQUAL(copy.raw(), TextAnchor::Start);

        TextAnchor assigned{};
        assigned = end;
        REQUIRE_EQUAL(assigned.raw(), TextAnchor::End);
    }

    void testEqualityAndInequalityOperators() {
        TextAnchor def1{}; // None
        TextAnchor def2{}; // None
        TextAnchor start{TextAnchor::Start};
        TextAnchor otherStart{TextAnchor::Start};
        TextAnchor end{TextAnchor::End};

        // Equal values
        REQUIRE(def1 == def2);
        REQUIRE_FALSE(def1 != def2);

        REQUIRE(start == otherStart);
        REQUIRE_FALSE(start != otherStart);

        // Different values
        REQUIRE(start != end);
        REQUIRE_FALSE(start == end);

        REQUIRE(def1 != start);
        REQUIRE_FALSE(def1 == start);
    }

    void testToStringForMappedValues() {
        struct Case {
            TextAnchor::Value value;
            el::text::String expected;
        };

        const Case cases[] = {
            {TextAnchor::Start, "Start"_el},
            {TextAnchor::End, "End"_el},
            {TextAnchor::LineStart, "LineStart"_el},
            {TextAnchor::LineEnd, "LineEnd"_el},
            {TextAnchor::UnicodeWordBoundary, "UnicodeWordBoundary"_el},
            {TextAnchor::AsciiWordBoundary, "AsciiWordBoundary"_el},
            {TextAnchor::NonUnicodeWordBoundary, "NonUnicodeWordBoundary"_el},
            {TextAnchor::NonAsciiWordBoundary, "NonAsciiWordBoundary"_el},
        };

        for (const auto &c : cases) {
            TextAnchor anchor{c.value};
            const auto name = anchor.toString();
            REQUIRE_EQUAL(name, c.expected);
        }
    }

    void testToStringForUnmappedValues() {
        // The None value is not present in the name map and must return an empty string
        TextAnchor noneDefault{}; // default is None
        REQUIRE_EQUAL(noneDefault.raw(), TextAnchor::None);

        const auto nameDefault = noneDefault.toString();
        REQUIRE(nameDefault.isEmpty());

        // Explicit None construction should behave the same
        TextAnchor noneExplicit{TextAnchor::None};
        const auto nameExplicit = noneExplicit.toString();
        REQUIRE(nameExplicit.isEmpty());

        // An invalid enum value (not present in the map) must also return an empty string
        auto invalidValue = static_cast<TextAnchor::Value>(0xFF);
        TextAnchor invalid{invalidValue};
        const auto nameInvalid = invalid.toString();
        REQUIRE(nameInvalid.isEmpty());
    }
};
