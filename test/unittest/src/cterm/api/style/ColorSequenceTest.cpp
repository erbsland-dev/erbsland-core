// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(ColorSequence)
class ColorSequenceTest final : public el::UnitTest {
public:
    void testDefaultSequenceIsEmpty() {
        const auto sequence = ColorSequence{};
        REQUIRE(sequence.empty());
        REQUIRE_EQUAL(sequence.entryCount(), 0U);
        REQUIRE_EQUAL(sequence.sequenceLength(), 0U);
        REQUIRE_EQUAL(sequence.color(0U), Color{});
        REQUIRE_EQUAL(sequence.colorNormalized(0.5), Color{});
    }

    void testAddAndCyclicLookup() {
        auto sequence = ColorSequence{};
        sequence.add(Color{fg::Black, bg::Black}, 20);
        sequence.add(Color{fg::Blue, bg::Black});
        sequence.add(Color{fg::White, bg::Black});
        sequence.add(Color{fg::Blue, bg::Black});

        REQUIRE_FALSE(sequence.empty());
        REQUIRE_EQUAL(sequence.entryCount(), 4U);
        REQUIRE_EQUAL(sequence.sequenceLength(), 23U);
        REQUIRE_EQUAL(sequence.color(0), Color(fg::Black, bg::Black));
        REQUIRE_EQUAL(sequence.color(19), Color(fg::Black, bg::Black));
        REQUIRE_EQUAL(sequence.color(20), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(sequence.color(21), Color(fg::White, bg::Black));
        REQUIRE_EQUAL(sequence.color(22), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(sequence.color(23), Color(fg::Black, bg::Black));
    }

    void testConstructionFromCodeInitializers() {
        const auto colorListSequence = ColorSequence{
            Color{fg::Red, bg::Black},
            Color{fg::Green, bg::Black},
            Color{fg::Blue, bg::Black},
        };
        REQUIRE_EQUAL(colorListSequence.sequenceLength(), 3U);
        REQUIRE_EQUAL(colorListSequence.color(0), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(colorListSequence.color(1), Color(fg::Green, bg::Black));
        REQUIRE_EQUAL(colorListSequence.color(2), Color(fg::Blue, bg::Black));

        const auto countedSequence = ColorSequence{
            {Color{fg::BrightWhite, bg::Black}, 2},
            {Color{fg::BrightYellow, bg::Black}, 4},
            {Color{fg::Yellow, bg::Black}, 3},
        };
        REQUIRE_EQUAL(countedSequence.sequenceLength(), 9U);
        REQUIRE_EQUAL(countedSequence.color(0), Color(fg::BrightWhite, bg::Black));
        REQUIRE_EQUAL(countedSequence.color(1), Color(fg::BrightWhite, bg::Black));
        REQUIRE_EQUAL(countedSequence.color(2), Color(fg::BrightYellow, bg::Black));
        REQUIRE_EQUAL(countedSequence.color(5), Color(fg::BrightYellow, bg::Black));
        REQUIRE_EQUAL(countedSequence.color(6), Color(fg::Yellow, bg::Black));
        REQUIRE_EQUAL(countedSequence.color(8), Color(fg::Yellow, bg::Black));
    }

    void testNormalizedLookupWithClamping() {
        const auto sequence = ColorSequence{
            {Color{fg::Black, bg::Black}, 2},
            {Color{fg::Blue, bg::Black}, 1},
            {Color{fg::White, bg::Black}, 1},
        };

        REQUIRE_EQUAL(sequence.colorNormalized(-1.0), Color(fg::Black, bg::Black));
        REQUIRE_EQUAL(sequence.colorNormalized(0.0), Color(fg::Black, bg::Black));
        REQUIRE_EQUAL(sequence.colorNormalized(0.49), Color(fg::Black, bg::Black));
        REQUIRE_EQUAL(sequence.colorNormalized(0.70), Color(fg::Blue, bg::Black));
        REQUIRE_EQUAL(sequence.colorNormalized(1.0), Color(fg::White, bg::Black));
        REQUIRE_EQUAL(sequence.colorNormalized(1.5), Color(fg::White, bg::Black));
    }
};
