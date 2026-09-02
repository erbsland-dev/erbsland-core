// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestHelper.hpp"
#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/impl/StandardInput.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string_view>
#include <utility>

TESTED_TARGETS(Backend readStandardInputLine)
class BackendTest final : public UNITTEST_SUBCLASS(TerminalTestHelper) {
    class LineInputStream final : public el::stream::TextInputStream {
    public:
        explicit LineInputStream(el::text::String line) : _line{std::move(line)} {}

    public: // implement TextInputStream
        [[nodiscard]] auto encoding() const noexcept -> el::text::StringEncoding override {
            return el::text::StringEncoding::Utf8;
        }
        [[nodiscard]] auto effectiveEncoding() const noexcept -> el::text::StringEncoding override {
            return el::text::StringEncoding::Utf8;
        }
        [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

        [[nodiscard]] auto readChar() -> el::stream::StreamReadResult<el::text::Char> override {
            return {el::stream::StreamReadStatus::Finished, el::text::Char{}};
        }
        [[nodiscard]] auto read(el::unit::CpLength) -> el::stream::StreamReadResult<el::text::String> override {
            return {el::stream::StreamReadStatus::Finished, el::text::String{}};
        }
        [[nodiscard]] auto readLine(el::unit::CpLength) -> el::stream::StreamReadResult<el::text::String> override {
            if (_wasRead) {
                return {el::stream::StreamReadStatus::Finished, el::text::String{}};
            }
            _wasRead = true;
            if (_line.isEmpty()) {
                return {el::stream::StreamReadStatus::Finished, el::text::String{}};
            }
            return {el::stream::StreamReadStatus::Data, _line};
        }
        [[nodiscard]] auto readAll(el::unit::CpLength) -> el::stream::StreamReadResult<el::text::String> override {
            return {el::stream::StreamReadStatus::Finished, el::text::String{}};
        }

    private:
        el::text::String _line;
        bool _wasRead{false};
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

public:
    void testStandardInputLineEndings() {
        REQUIRE_EQUAL(readLine("Grüezi\n"_el), "Grüezi"_el);
        REQUIRE_EQUAL(readLine("first\r\n"_el), "first"_el);
        REQUIRE_EQUAL(readLine("second\r"_el), "second"_el);
        REQUIRE_EQUAL(readLine("without line ending"_el), "without line ending"_el);
        REQUIRE_EQUAL(readLine("content\r\r\n"_el), "content\r"_el);
        REQUIRE(readLine({}).isEmpty());
    }

    void testTerminalUsesEmitColorWhenColorCodesAreUnavailable() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        auto terminal = createTerminal(backend);

        REQUIRE_FALSE(terminal->lineBufferEnabled());

        terminal->write(Block{U'A', fg::Red, bg::Blue});
        terminal->flush();

        REQUIRE_EQUAL(backend->_emittedColors.size(), std::size_t{1});
        REQUIRE_EQUAL(backend->_emittedColors[0], Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(backend->output(), std::string{"A"});
        const auto escapePosition = backend->output().find("\x1b[");
        REQUIRE_EQUAL(escapePosition, std::string::npos);
    }

    void testTerminalUsesCursorFallbackHooksWhenCursorCodesAreUnavailable() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorCodes = false;
        auto terminal = createTerminal(backend);

        REQUIRE_FALSE(terminal->lineBufferEnabled());

        terminal->moveLeft(blockCoordinate(2));
        terminal->moveRight(blockCoordinate(3));
        terminal->moveUp(blockCoordinate(4));
        terminal->moveDown(blockCoordinate(5));
        terminal->moveTo(block::Position{6, 7});
        terminal->moveHome();
        terminal->clearScreen();
        terminal->flush();

        REQUIRE_EQUAL(backend->output(), std::string{});
        REQUIRE_EQUAL(backend->_clearScreenCallCount, 1);
        REQUIRE_EQUAL(backend->_cursorMoves.size(), std::size_t{6});
        REQUIRE_EQUAL(
            backend->_cursorMoves[0], (TerminalTestBackend::CursorMove{block::Position{-2, 0}, MoveMode::Relative}));
        REQUIRE_EQUAL(
            backend->_cursorMoves[1], (TerminalTestBackend::CursorMove{block::Position{3, 0}, MoveMode::Relative}));
        REQUIRE_EQUAL(
            backend->_cursorMoves[2], (TerminalTestBackend::CursorMove{block::Position{0, -4}, MoveMode::Relative}));
        REQUIRE_EQUAL(
            backend->_cursorMoves[3], (TerminalTestBackend::CursorMove{block::Position{0, 5}, MoveMode::Relative}));
        REQUIRE_EQUAL(
            backend->_cursorMoves[4], (TerminalTestBackend::CursorMove{block::Position{6, 7}, MoveMode::Absolute}));
        REQUIRE_EQUAL(
            backend->_cursorMoves[5], (TerminalTestBackend::CursorMove{block::Position{0, 0}, MoveMode::Absolute}));
    }

    void testTerminalUsesCursorVisibilityFallbackHookWhenVisibilityCodesAreUnavailable() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorVisibilityCodes = false;
        auto terminal = createTerminal(backend);

        terminal->setCursorVisible(false);
        terminal->flush();

        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{1});
        REQUIRE_FALSE(backend->_cursorVisibilityChanges[0]);
        REQUIRE_EQUAL(backend->output(), std::string{});

        backend->clearRecordedOperations();
        terminal->setCursorVisible(true);
        terminal->flush();

        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{1});
        REQUIRE(backend->_cursorVisibilityChanges[0]);
        REQUIRE_EQUAL(backend->output(), std::string{});
    }

    void testTextOutputModeSuppressesBackendControlHooks() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        backend->_supportsCursorCodes = false;
        backend->_supportsCursorVisibilityCodes = false;
        auto terminal = createTerminal(backend, block::Size{2, 1});

        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        terminal->clearScreen();
        terminal->moveHome();
        terminal->setCursorVisible(false);
        terminal->setColor(Color{fg::Red, bg::Blue});
        terminal->updateScreen(createBuffer({"HI"}));

        REQUIRE_EQUAL(backend->output(), std::string{"HI\n"});
        REQUIRE_EQUAL(backend->_emittedColors.size(), std::size_t{0});
        REQUIRE_EQUAL(backend->_cursorMoves.size(), std::size_t{0});
        REQUIRE_EQUAL(backend->_clearScreenCallCount, 0);
        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{0});
    }

    void testRestoreScreenUsesBackendFallbacksWhenCodesAreUnavailable() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        backend->_supportsCursorVisibilityCodes = false;
        auto terminal = createTerminal(backend);

        terminal->setColor(Color{fg::Red, bg::Blue});
        terminal->flush();
        backend->clearRecordedOperations();

        terminal->restoreScreen();

        REQUIRE_EQUAL(backend->_emittedColors.size(), std::size_t{1});
        REQUIRE_EQUAL(backend->_emittedColors[0], Color::reset());
        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{1});
        REQUIRE(backend->_cursorVisibilityChanges[0]);
        REQUIRE_EQUAL(backend->output(), std::string{});
        REQUIRE_EQUAL(backend->_restorePlatformCallCount, 1);
    }

    void testResizeClearsTheScreenExactlyOnceBeforeTheNextUpdate() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorCodes = false;
        auto terminal = createTerminal(backend, block::Size{4, 1});

        terminal->updateScreen(createBuffer({"ABCD"}));
        backend->clearRecordedOperations();

        terminal->setSize(block::Size{5, 1});
        terminal->updateScreen(createBuffer({"ABCDE"}));

        REQUIRE_EQUAL(backend->_clearScreenCallCount, 1);
        REQUIRE_EQUAL(backend->_cursorMoves.size(), std::size_t{1});
        REQUIRE_EQUAL(
            backend->_cursorMoves[0], (TerminalTestBackend::CursorMove{block::Position{0, 0}, MoveMode::Absolute}));
        REQUIRE_EQUAL(backend->output(), std::string{"ABCDE"});
    }

    void testTerminalUsesEmitBlockAttributesWhenAnsiAttributeCodesAreUnavailable() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportedBlockAttributes = BlockAttributes::fromMask(BlockAttributes::Underline.value);
        backend->_supportedBlockAttributeCodes = BlockAttributes::fromMask(0);
        auto terminal = createTerminal(backend);

        REQUIRE_FALSE(terminal->lineBufferEnabled());

        terminal->setUnderline(true);
        terminal->write(Block{U'A'});
        terminal->flush();

        REQUIRE_EQUAL(backend->_emittedBlockAttributes.size(), std::size_t{1});
        REQUIRE(backend->_emittedBlockAttributes[0].isUnderline());
        REQUIRE_EQUAL(backend->output(), std::string{"A"});
        const auto underlinePosition = backend->output().find("\x1b[4m");
        REQUIRE_EQUAL(underlinePosition, std::string::npos);
    }

    void testUnsupportedCharacterAttributesAreIgnored() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportedBlockAttributes = BlockAttributes::fromMask(0);
        backend->_supportedBlockAttributeCodes = BlockAttributes::fromMask(0);
        auto terminal = createTerminal(backend);

        terminal->setBold(true);

        REQUIRE_FALSE(terminal->blockAttributes().isBold());
        REQUIRE_EQUAL(backend->_emittedBlockAttributes.size(), std::size_t{0});
        const auto boldPosition = backend->output().find("\x1b[1m");
        REQUIRE_EQUAL(boldPosition, std::string::npos);
    }

private:
    [[nodiscard]] static auto readLine(el::text::String line) -> el::text::String {
        auto redirect = el::stream::redirectStdIn(std::make_shared<LineInputStream>(std::move(line)));
        return el::cterm::impl::readStandardInputLine();
    }
};
