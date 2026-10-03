// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cterm/Buffer.hpp>
#include <erbsland/cterm/BufferConstRefView.hpp>
#include <erbsland/cterm/BufferRenderOptions.hpp>
#include <erbsland/cterm/TerminalEmulation.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace erbsland;
using namespace erbsland::cterm;
using namespace erbsland::text::literals;

TESTED_TARGETS(ReadableBuffer BufferRenderOptions RenderFormat)
class BufferRenderTest final : public el::UnitTest {
public:
    void testPlainDimensionsAndTrimming() {
        auto buffer = Buffer{block::Size{4, 3}};
        buffer.set({0, 0}, Block{U'A'});
        auto options = BufferRenderOptions{}.setFormat(RenderFormat::Text);
        REQUIRE_EQUAL(buffer.toString(options), "A   \n    \n    "_el);
        options.setTrimTrailingSpaces(true);
        REQUIRE_EQUAL(buffer.toString(options), "A\n\n"_el);
        options.setTrimTrailingEmptyLines(true);
        REQUIRE_EQUAL(buffer.toString(options), "A"_el);
    }

    void testStyledBlanksAreRetainedAndStylesRoundTrip() {
        auto buffer = Buffer{block::Size{3, 2}};
        buffer.set({0, 0}, Block{U'A', fg::Red, bg::Blue});
        buffer.set({2, 1}, Block{U' ', fg::Default, bg::Green});
        const auto options = BufferRenderOptions{}.setTrimTrailingSpaces(true).setTrimTrailingEmptyLines(true);
        const auto rendered = buffer.toString(options);
        REQUIRE(rendered.startsWith("\x1b[0m"_el));
        REQUIRE(rendered.endsWith("\x1b[0m"_el));
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({3, 2})};
        emulator.write(rendered);
        emulator.finish();
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).color(), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(emulator.activeBuffer().get({2, 1}).color(), Color(fg::Default, bg::Green));
    }

    void testClippedWideCellsPreserveViewDimensions() {
        auto buffer = Buffer{block::Size{4, 1}};
        buffer.set({0, 0}, Block{"界"_el});
        const auto options = BufferRenderOptions{}.setFormat(RenderFormat::Text);
        const auto continuation = BufferConstRefView{buffer, block::Rectangle{1, 0, 2, 1}};
        REQUIRE_EQUAL(continuation.toString(options), "  "_el);
        const auto leading = BufferConstRefView{buffer, block::Rectangle{0, 0, 1, 1}};
        REQUIRE_EQUAL(leading.toString(options), " "_el);
    }

    void testWideCharactersAndEmptyBuffers() {
        auto buffer = Buffer{block::Size{4, 1}};
        buffer.set({0, 0}, Block{"界"_el});
        const auto options = BufferRenderOptions{}.setFormat(RenderFormat::Text).setTrimTrailingSpaces(true);
        REQUIRE_EQUAL(buffer.toString(options), "界"_el);
        buffer.fill(Block::space());
        REQUIRE_EQUAL(buffer.toString(BufferRenderOptions{}.setTrimTrailingEmptyLines(true)), ""_el);
    }
    void testAttributeTransitionsAndInheritedDefaultsRoundTrip() {
        auto buffer = Buffer{block::Size{3, 1}};
        auto first = BlockAttributes::reset();
        first.setBold(true);
        first.setDim(true);
        first.setUnderline(true);
        auto second = first;
        second.setBold(false);
        second.setUnderline(false);
        buffer.set({0, 0}, Block{U'A'}.withAttributes(first));
        buffer.set({1, 0}, Block{U'B'}.withAttributes(second));
        buffer.set({2, 0}, Block{U'C'});
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({3, 1})};
        emulator.write(buffer.toString());
        emulator.finish();
        REQUIRE_EQUAL(emulator.activeBuffer().get({0, 0}).attributes(), first);
        REQUIRE_EQUAL(emulator.activeBuffer().get({1, 0}).attributes(), second);
        REQUIRE_EQUAL(emulator.activeBuffer().get({2, 0}).attributes(), BlockAttributes::reset());
        REQUIRE_EQUAL(buffer.toString(BufferRenderOptions{}.setFormat(RenderFormat::Text)), "ABC"_el);
    }
};
