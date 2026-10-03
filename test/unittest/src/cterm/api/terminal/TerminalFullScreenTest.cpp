// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestHelper.hpp"

#include <erbsland/cterm/TerminalEmulation.hpp>

TESTED_TARGETS(Terminal)
class TerminalFullScreenTest final : public UNITTEST_SUBCLASS(TerminalTestHelper) {
public:
    void testFullFramesDoNotScrollAtTheBottomRightCell() {
        for (const auto backBuffer : {false, true}) {
            const auto backend = std::make_shared<TerminalTestBackend>();
            backend->_supportsCursorCodes = true;
            auto terminal = Terminal{backend, block::Size{5, 2}};
            terminal.setBackBufferEnabled(backBuffer);
            auto buffer = createBuffer({"ABCDE", "FGHIJ"});
            auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({5, 2})};
            terminal.updateScreen(buffer, settings());
            emulator.write(el::String{backend->output()});
            REQUIRE_FALSE(emulator.hasScrollback());
            REQUIRE_EQUAL(emulator.regularBuffer().get({4, 0}).singleOrNull(), el::Char{U'E'});
            REQUIRE_EQUAL(emulator.regularBuffer().get({4, 1}).singleOrNull(), el::Char{U'J'});
            backend->clearOutput();
            buffer.set({4, 1}, Block{U'!'});
            terminal.updateScreen(buffer, settings());
            emulator.write(el::String{backend->output()});
            REQUIRE_FALSE(emulator.hasScrollback());
            REQUIRE_EQUAL(emulator.regularBuffer().get({0, 0}).singleOrNull(), el::Char{U'A'});
            REQUIRE_EQUAL(emulator.regularBuffer().get({4, 1}).singleOrNull(), el::Char{U'!'});
        }
    }

    void testOneCellAndWideRightEdge() {
        WITH_CONTEXT(requireFrame(Buffer{block::Size{1, 1}, Block{U'X'}}));
        WITH_CONTEXT(requireFrame(Buffer::fromLinesInString(BlockString{"AB界\nCDéF"_el})));
    }

    void testResizeClearsAndRebuildsTheFullFrame() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorCodes = true;
        auto terminal = Terminal{backend, block::Size{5, 3}};
        terminal.setBackBufferEnabled(true);
        terminal.updateScreen(Buffer{block::Size{5, 3}, Block{U'X'}}, settings());
        backend->clearOutput();
        terminal.setSize({3, 2});
        terminal.updateScreen(Buffer{block::Size{3, 2}, Block{U'Y'}}, settings());
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize({3, 2})};
        emulator.write("old\nold"_el);
        emulator.write(el::String{backend->output()});
        REQUIRE_FALSE(emulator.hasScrollback());
        REQUIRE_EQUAL(emulator.regularBuffer().get({0, 0}).singleOrNull(), el::Char{U'Y'});
        REQUIRE_EQUAL(emulator.regularBuffer().get({2, 1}).singleOrNull(), el::Char{U'Y'});
    }

private:
    static auto settings() -> UpdateSettings {
        auto result = UpdateSettings{};
        result.setSwitchToAlternateBuffer(false);
        return result;
    }

    void requireFrame(const Buffer &buffer) {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorCodes = true;
        auto terminal = Terminal{backend, buffer.size()};
        terminal.updateScreen(buffer, settings());
        auto emulator = TerminalEmulation{TerminalEmulationOptions{}.setSize(buffer.size())};
        emulator.write(el::String{backend->output()});
        REQUIRE_FALSE(emulator.hasScrollback());
        buffer.size().forEach([&](const block::Position position) -> void {
            REQUIRE_EQUAL(emulator.regularBuffer().get(position).toString(), buffer.get(position).toString());
        });
    }
};
