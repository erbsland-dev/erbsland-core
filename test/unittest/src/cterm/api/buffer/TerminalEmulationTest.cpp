// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteRange.hpp>
#include <erbsland/cterm/TerminalEmulation.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace erbsland;
using namespace erbsland::cterm;
using namespace erbsland::text::literals;

TESTED_TARGETS(TerminalEmulation TerminalEmulationOptions CursorBuffer)
class TerminalEmulationTest final : public el::UnitTest {
public:
    void testEveryByteSplit() {
        const auto input =
            String{"hé界é\r\x1b[31;1mred\x1b[0m\nsecond\x1b[1;1H!\x1b]title\x1b\\\x1b[?1049halt\x1b[?1049l"_el};
        const auto options = TerminalEmulationOptions{}.setSize({12, 2}).setScrollbackLimit(block::Coordinate{4});
        auto expected = TerminalEmulation{options};
        expected.write(input);
        expected.finish();
        for (auto offset = unit::ByteIndex{0}; offset.distanceFromZero() <= input.length(); ++offset) {
            auto actual = TerminalEmulation{options};
            actual.write(input.slice(unit::ByteRange{unit::ByteIndex{0}, offset.distanceFromZero()}));
            actual.write(input.slice(unit::ByteRange{offset, input.length() - offset.distanceFromZero()}));
            actual.finish();
            REQUIRE_EQUAL(actual.regularBuffer().toString(), expected.regularBuffer().toString());
            REQUIRE_EQUAL(actual.alternateBuffer().toString(), expected.alternateBuffer().toString());
            REQUIRE_EQUAL(actual.activeBuffer().position(), expected.activeBuffer().position());
            REQUIRE_EQUAL(actual.scrollbackBuffer().toString(), expected.scrollbackBuffer().toString());
        }
    }

    void testFixedFramesAndBoundedScrollback() {
        auto emulator =
            TerminalEmulation{TerminalEmulationOptions{}.setSize({4, 2}).setScrollbackLimit(block::Coordinate{2})};
        REQUIRE_FALSE(emulator.hasScrollback());
        REQUIRE_EQUAL(emulator.scrollbackBuffer().size(), block::Size(4, 1));
        emulator.write("one\ntwo\ntri"_el);
        REQUIRE_EQUAL(emulator.regularBuffer().size(), block::Size(4, 2));
        REQUIRE_EQUAL(emulator.scrollbackBuffer().get({0, 0}).singleOrNull(), text::Char{U'o'});
        emulator.write("\x1b[H!"_el);
        REQUIRE_EQUAL(emulator.regularBuffer().get({0, 0}).singleOrNull(), text::Char{U'!'});
        REQUIRE_EQUAL(emulator.scrollbackBuffer().get({0, 0}).singleOrNull(), text::Char{U'o'});
        emulator.write("\x1b[2;1Hfour\nfive\nsix"_el);
        REQUIRE_EQUAL(emulator.scrollbackBuffer().size(), block::Size(4, 2));
        REQUIRE_EQUAL(emulator.scrollbackBuffer().get({0, 0}).singleOrNull(), text::Char{U'!'});
        REQUIRE_EQUAL(emulator.scrollbackBuffer().get({0, 1}).singleOrNull(), text::Char{U'f'});
        REQUIRE_EQUAL(emulator.regularBuffer().get({0, 1}).singleOrNull(), text::Char{U's'});
    }

    void testAlternateScrollingDoesNotEnterHistoryAndEraseClearsOnlyHistory() {
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({6, 1})};
        emulator.write("main\nframe\x1b[?1049halt\nnext\x1b[?1049l"_el);
        REQUIRE_EQUAL(emulator.scrollbackBuffer().get({0, 0}).singleOrNull(), text::Char{U'm'});
        REQUIRE_EQUAL(emulator.scrollbackBuffer().size().height(), block::Coordinate{1});
        emulator.write("\x1b[3J"_el);
        REQUIRE_FALSE(emulator.hasScrollback());
        REQUIRE_EQUAL(emulator.regularBuffer().get({0, 0}).singleOrNull(), text::Char{U'f'});
        emulator.write("\x1b[999S"_el);
        REQUIRE(emulator.hasScrollback());
        emulator.reset();
        REQUIRE_FALSE(emulator.hasScrollback());
        REQUIRE_EQUAL(emulator.scrollbackBuffer().size().height(), block::Coordinate{1});
    }

    void testAlternateScreensAndSavedState() {
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({8, 2})};
        emulator.write("main\x1b[?1049halt\x1b[?1049l!"_el);
        REQUIRE_FALSE(emulator.isAlternateScreenActive());
        REQUIRE_EQUAL(emulator.regularBuffer().get({4, 0}).singleOrNull(), text::Char{U'!'});
        REQUIRE_EQUAL(emulator.alternateBuffer().get({0, 0}).singleOrNull(), text::Char{U'a'});
        emulator.write("\x1b[?47h"_el);
        REQUIRE(emulator.isAlternateScreenActive());
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).singleOrNull(), text::Char{U'a'});
        emulator.write("\x1b[?47l\x1b[?1047h"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).singleOrNull(), text::Char{U'a'});
        emulator.write("\x1b[?1047l\x1b[?47h"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).singleOrNull(), text::Char{U' '});
        emulator.write("\x1b[?47l\x1b[2;3H\x1b[?1048h\x1b[H\x1b[?1048l!"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().get({2, 1}).singleOrNull(), text::Char{U'!'});
    }

    void testResetControlPreservesFollowingInput() {
        auto emulator = TerminalEmulation{};
        REQUIRE_EQUAL(emulator.regularBuffer().size(), block::Size(120, 25));
        REQUIRE_EQUAL(emulator.alternateBuffer().size(), block::Size(120, 25));
        emulator.write(
            "before\x1b"
            "cafter"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).singleOrNull(), text::Char{U'a'});
        REQUIRE_EQUAL(emulator.activeBuffer().get({4, 0}).singleOrNull(), text::Char{U'r'});
    }

    void testIncompleteInputAndUnsupportedSequences() {
        auto emulator = TerminalEmulation{};
        emulator.write("A\x1b[38;2;1;2;3mB\x1bPpayload\x1b\\C\x1b["_el);
        emulator.finish();
        REQUIRE_EQUAL(emulator.activeBuffer().get({2, 0}).singleOrNull(), text::Char{U'C'});
        REQUIRE_FALSE(emulator.activeBuffer().get({1, 0}).attributes().isBold());
        emulator.reset();
        emulator.write("\x1bPignored\x18visible"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).singleOrNull(), text::Char{U'v'});
        emulator.reset();
        const auto bytes = el::unittest::th::stdStringFromHex("e2 82");
        emulator.write(String{bytes});
        emulator.finish();
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).first(), text::Char{U'�'});
    }
    void testOptionsAreCopiedAndControlsRespectPolicies() {
        auto options = TerminalEmulationOptions{}
                           .setSize({5, 2})

                           .setAutoWrap(false)
                           .setTabWidth(block::Coordinate{2})
                           .setLineFeedReturnsToColumnZero(false);
        auto emulator = TerminalEmulation{options};
        options.setSize({8, 4});
        emulator.write("A\tB\nCXYZ"_el);
        REQUIRE_EQUAL(emulator.activeBuffer().size(), block::Size(5, 2));
        REQUIRE_EQUAL(emulator.activeBuffer().get({2, 0}).singleOrNull(), text::Char{U'B'});
        REQUIRE_EQUAL(emulator.activeBuffer().get({3, 1}).singleOrNull(), text::Char{U'C'});
        REQUIRE_EQUAL(emulator.activeBuffer().get({4, 1}).singleOrNull(), text::Char{U'Z'});
        emulator.finish();
        emulator.finish();
        REQUIRE_THROWS(emulator.write("after EOF"_el));
        emulator.reset();
        REQUIRE_EQUAL(emulator.activeBuffer().position(), block::Position(0, 0));
        REQUIRE_THROWS(TerminalEmulationOptions{}.setTabWidth(block::Coordinate{0}));
        REQUIRE_THROWS(TerminalEmulationOptions{}.setSize({0, 1}));
    }

    void testMalformedUtf8IsIndependentOfChunkBoundaries() {
        const auto bytes = String{el::unittest::th::stdStringFromHex("41 f0 28 8c 28 e2 82")};
        auto expected = TerminalEmulation{};
        expected.write(bytes);
        expected.finish();
        auto actual = TerminalEmulation{};
        for (auto index = unit::ByteIndex{0}; index.distanceFromZero() < bytes.length(); ++index) {
            actual.write(bytes.slice(unit::ByteRange{index, unit::ByteCount{1}}));
        }
        actual.finish();
        REQUIRE_EQUAL(actual.activeBuffer().toString(), expected.activeBuffer().toString());
    }
    void testMatchesLegacyConverterTextFixtures() {
        // Golden captures from erbsland-ansi-convert 0.9.1; no converter is needed to run these tests.
        compareLegacyFixture("progress:1\rprogress:2"_el, "progress:2"_el);
        compareLegacyFixture("abc\bX"_el, "abX"_el);
        compareLegacyFixture("abcdef\r\x1b[KOK"_el, "OK"_el);
        compareLegacyFixture("\x1b[31mred\x1b[0m"_el, "red"_el);
        compareLegacyFixture("main\x1b[?1049halt\x1b[?1049l!"_el, "main!"_el);
    }

private:
    void compareLegacyFixture(const String &input, const String &expected) {
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({12, 3})};
        emulator.write(input);
        emulator.finish();
        const auto options = BufferRenderOptions{}
                                 .setFormat(RenderFormat::Text)
                                 .setTrimTrailingSpaces(true)
                                 .setTrimTrailingEmptyLines(true);
        REQUIRE_EQUAL(emulator.activeBuffer().toString(options), expected);
    }
};
