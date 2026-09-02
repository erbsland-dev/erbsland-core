// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Terminal)
class TerminalTest final : public UNITTEST_SUBCLASS(TerminalTestHelper) {
public:
    void testConstructionOverloadsAreUnambiguous() {
        static_assert(requires { Terminal{}; });
        static_assert(requires { Terminal{TerminalFlags{}}; });
        static_assert(requires { Terminal{block::Size{80, 25}}; });
        static_assert(requires { Terminal{block::Size{80, 25}, TerminalFlags{TerminalFlag::NoSignalHandling}}; });
        static_assert(requires(BackendPtr backend) { Terminal{backend}; });
        static_assert(requires(BackendPtr backend) { Terminal{backend, block::Size{80, 25}}; });

        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto defaultSizedTerminal = Terminal{backend};
        REQUIRE_EQUAL(defaultSizedTerminal.size(), block::Size(80, 25));

        const auto clampedSizedTerminal = Terminal{backend, block::Size{0, 5'000}};
        REQUIRE_EQUAL(clampedSizedTerminal.size(), block::Size(1, 2'048));
    }

    void testInputDelegatesToTheActiveBackend() {
        const auto firstBackend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(firstBackend);
        auto &input = terminal->input();

        REQUIRE_EQUAL(input.mode(), Input::Mode::ReadLine);

        input.setMode(Input::Mode::Key);
        REQUIRE_EQUAL(firstBackend->_setInputModeCallCount, 1);
        REQUIRE_EQUAL(firstBackend->_inputMode, Input::Mode::Key);
        REQUIRE_EQUAL(input.mode(), Input::Mode::Key);

        firstBackend->_readKeyResults.push(Key{Key::F5});
        REQUIRE_EQUAL(input.readKey(std::chrono::milliseconds{125}), Key{Key::F5});
        REQUIRE_EQUAL(firstBackend->_readKeyCallCount, 1);
        REQUIRE_EQUAL(firstBackend->_readKeyTimeouts.size(), std::size_t{1});
        REQUIRE_EQUAL(firstBackend->_readKeyTimeouts[0], std::chrono::milliseconds{125});

        firstBackend->_waitForKeyResults.push(Key{Key::Enter});
        REQUIRE_EQUAL(input.waitForKey(), Key{Key::Enter});
        REQUIRE_EQUAL(firstBackend->_waitForKeyCallCount, 1);

        firstBackend->_readLineResults.push("first line"_el);
        REQUIRE_EQUAL(input.readLine(), "first line"_el);
        REQUIRE_EQUAL(firstBackend->_readLineCallCount, 1);

        const auto secondBackend = std::make_shared<TerminalTestBackend>();
        terminal->setBackend(secondBackend);

        REQUIRE_EQUAL(&terminal->input(), &input);
        REQUIRE_EQUAL(input.mode(), Input::Mode::ReadLine);

        input.setMode(Input::Mode::Key);
        REQUIRE_EQUAL(secondBackend->_setInputModeCallCount, 1);
        REQUIRE_EQUAL(secondBackend->_inputMode, Input::Mode::Key);
        REQUIRE_EQUAL(firstBackend->_setInputModeCallCount, 1);

        secondBackend->_readKeyResults.push(Key{Key::Escape});
        REQUIRE_EQUAL(input.readKey(std::chrono::milliseconds{250}), Key{Key::Escape});
        REQUIRE_EQUAL(secondBackend->_readKeyCallCount, 1);
        REQUIRE_EQUAL(secondBackend->_readKeyTimeouts.size(), std::size_t{1});
        REQUIRE_EQUAL(secondBackend->_readKeyTimeouts[0], std::chrono::milliseconds{250});
        REQUIRE_EQUAL(firstBackend->_readKeyCallCount, 1);

        secondBackend->_waitForKeyResults.push(Key{Key::Backspace});
        REQUIRE_EQUAL(input.waitForKey(), Key{Key::Backspace});
        REQUIRE_EQUAL(secondBackend->_waitForKeyCallCount, 1);
        REQUIRE_EQUAL(firstBackend->_waitForKeyCallCount, 1);

        secondBackend->_readLineResults.push("second line"_el);
        REQUIRE_EQUAL(input.readLine(), "second line"_el);
        REQUIRE_EQUAL(secondBackend->_readLineCallCount, 1);
        REQUIRE_EQUAL(firstBackend->_readLineCallCount, 1);
    }

    void testInitializeScreenUsesDetectedSizeAndSafeMargin() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorVisibilityCodes = false;
        backend->_detectedScreenSize = block::Size{6, 4};
        auto terminal = createTerminal(backend, block::Size{80, 25});

        terminal->initializeScreen();

        REQUIRE_EQUAL(backend->_initializePlatformCallCount, 1);
        REQUIRE_EQUAL(backend->_detectScreenSizeCallCount, 1);
        REQUIRE_EQUAL(backend->_emitFlushCallCount, 1);
        REQUIRE_EQUAL(terminal->size(), block::Size(5, 3));
        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{1});
        REQUIRE_FALSE(backend->_cursorVisibilityChanges[0]);
        REQUIRE_EQUAL(backend->output(), std::string{});

        terminal->setSafeMarginEnabled(false);
        REQUIRE_EQUAL(terminal->size(), block::Size(6, 4));

        backend->_detectedScreenSize = block::Size{9, 7};
        terminal->testScreenSize();
        REQUIRE_EQUAL(terminal->size(), block::Size(9, 7));
    }

    void testIsInteractiveDelegatesToTheBackend() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_isInteractive = false;
        auto terminal = createTerminal(backend);

        terminal->initializeScreen();
        REQUIRE_FALSE(terminal->isInteractive());

        backend->_isInteractive = true;
        REQUIRE(terminal->isInteractive());
    }

    void testNonInteractiveLifecycleEmitsNoTerminalControlSequences() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_isInteractive = false;
        auto terminal = createTerminal(backend);

        terminal->initializeScreen();
        terminal->restoreScreen();

        REQUIRE_EQUAL(backend->output(), std::string{});
        REQUIRE(backend->_cursorVisibilityChanges.empty());
        REQUIRE(backend->_alternateScreenBufferChanges.empty());
        REQUIRE_EQUAL(backend->_initializePlatformCallCount, 1);
        REQUIRE_EQUAL(backend->_restorePlatformCallCount, 1);
    }

    void testTextOutputModeFallsBackToPlainTextAndLocksAnsiFeatures() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{2, 1});
        auto buffer = createBuffer({"HI"});

        terminal->setRefreshMode(Terminal::RefreshMode::Clear);
        terminal->setBackBufferEnabled(true);
        REQUIRE(terminal->backBufferEnabled());

        terminal->setOutputMode(Terminal::OutputMode::BlockText);

        REQUIRE_EQUAL(terminal->outputMode(), Terminal::OutputMode::BlockText);
        REQUIRE_FALSE(terminal->backBufferEnabled());
        REQUIRE_FALSE(terminal->sizeDetectionEnabled());
        REQUIRE_EQUAL(terminal->refreshMode(), Terminal::RefreshMode::Keep);

        terminal->setLineBufferEnabled(false);
        REQUIRE_FALSE(terminal->lineBufferEnabled());
        terminal->setLineBufferEnabled(true);
        REQUIRE_FALSE(terminal->lineBufferEnabled());

        terminal->setSizeDetectionEnabled(true);
        REQUIRE_FALSE(terminal->sizeDetectionEnabled());

        terminal->updateScreen(buffer);
        REQUIRE_EQUAL(backend->output(), std::string{"HI\n"});
        REQUIRE_EQUAL(backend->_emitFlushCallCount, 1);

        backend->clearOutput();
        terminal->clearScreen();
        terminal->moveHome();
        terminal->setColor(Color{fg::Red, bg::Blue});
        terminal->write(Block{U'A', fg::Red, bg::Blue});
        terminal->writeLineBreak();
        terminal->flush();
        REQUIRE_EQUAL(backend->output(), std::string{"A\n"});
    }

    void testPrintParagraphResetsTheBackgroundBeforeNewlinesInFullControlMode() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 4});
        auto paragraph = BlockStringEditor{};
        paragraph.append(bg::Blue, "AB CD"_el);
        auto options = ParagraphOptions{};
        options.setBackgroundMode(ParagraphBackgroundMode::FullRight);

        const auto writtenLines = terminal->printParagraph(paragraph, options);
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[44mAB  \x1b[49m\n\x1b[44mCD  \x1b[49m\n"});
    }

    void testPrintParagraphAcceptsStringSlices() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 4});
        const auto source = BlockStringEditor{"xAB CD!"_el};

        const auto writtenLines =
            terminal->printParagraph(BlockString{source}.slice(BlockRange{BlockIndex{1U}, BlockCount{5U}}));
        terminal->flush();

        REQUIRE_EQUAL(writtenLines, 2);
        REQUIRE_EQUAL(backend->output(), std::string{"AB\nCD\n"});
    }

    void testLineBufferCanBeDisabledForImmediateEmission() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);

        terminal->write("A"_el);
        REQUIRE_EQUAL(backend->output(), std::string{});

        terminal->flush();
        REQUIRE_EQUAL(backend->output(), std::string{"A"});

        backend->clearOutput();
        terminal->setLineBufferEnabled(false);
        terminal->write("B"_el);
        REQUIRE_EQUAL(backend->output(), std::string{"B"});
    }

    void testWriteBufferResetsTheColorAfterEachLine() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend);
        auto buffer = Buffer{block::Size{1, 2}};
        buffer.set(block::Position{0, 0}, Block{U'A', fg::Red, bg::Black});
        buffer.set(block::Position{0, 1}, Block{U'B', fg::Green, bg::Black});

        terminal->write(buffer);
        terminal->flush();

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[31;40mA\x1b[39;49m\n\x1b[32;40mB\x1b[39;49m\n"});
    }

    void testUpdateScreenAppliesCropMarksAndRefreshMode() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{2, 2});
        auto buffer = createBuffer({
            "ABC",
            "DEF",
            "GHI",
        });
        auto settings = UpdateSettings{};
        settings.setShowCropMarks(true);
        settings.setCropMarkRight(Block{U'>'});
        settings.setCropMarkBottom(Block{U'v'});
        settings.setCropMarkBottomRight(Block{U'x'});
        settings.setSwitchToAlternateBuffer(false);
        terminal->setRefreshMode(Terminal::RefreshMode::Clear);

        terminal->updateScreen(buffer, settings);

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[2J\x1b[1;1H\x1b[?7lA>\x1b[2;1Hvx\x1b[2;1H\x1b[?7h"});
    }

    void testUpdateScreenDisplaysTheMinimumSizeMessageOnTheFirstFrame() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});
        const auto settings = createMinimumSizeWarningSettings(block::Size{6, 2}, "HEY"_el);

        terminal->updateScreen(createBuffer({"ABCDE"}), settings);

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[H\x1b[?7l.HEY.\x1b[1;1H\x1b[?7h"});
    }

    void testUpdateScreenRefreshesTheMinimumSizeMessageWhenTheSettingsChange() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});

        terminal->updateScreen(createBuffer({"ABCDE"}), createMinimumSizeWarningSettings(block::Size{6, 2}, "ONE"_el));
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[H\x1b[?7l.ONE.\x1b[1;1H\x1b[?7h"});

        backend->clearOutput();
        terminal->updateScreen(createBuffer({"ABCDE"}), createMinimumSizeWarningSettings(block::Size{6, 2}, "TWO"_el));
        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[H\x1b[?7l.TWO.\x1b[1;1H\x1b[?7h"});
    }

    void testUpdateScreenRendersTheActualContentAtTheConfiguredMinimumSize() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});

        terminal->updateScreen(createBuffer({"ABCDE"}), createMinimumSizeWarningSettings(block::Size{5, 1}, "HEY"_el));

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[H\x1b[?7lABCDE\x1b[1;1H\x1b[?7h"});
    }

    void testUpdateScreenSwitchesToTheAlternateBufferByDefault() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});

        terminal->updateScreen(createBuffer({"ABCDE"}));

        REQUIRE(terminal->isAlternateScreenActive());
        REQUIRE(backend->_isAlternateScreenActive);
        REQUIRE_EQUAL(backend->_alternateScreenBufferChanges.size(), std::size_t{1});
        REQUIRE(backend->_alternateScreenBufferChanges[0]);

        const auto output = backend->output();
        const auto alternateBufferPos = output.find("\x1b[?1049h");
        const auto textPos = output.find("ABCDE");
        REQUIRE_NOT_EQUAL(alternateBufferPos, std::string::npos);
        REQUIRE_NOT_EQUAL(textPos, std::string::npos);
        REQUIRE_LESS(alternateBufferPos, textPos);
    }

    void testUpdateScreenCanKeepTheMainBufferWhenConfigured() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});
        auto settings = UpdateSettings{};
        settings.setSwitchToAlternateBuffer(false);

        terminal->updateScreen(createBuffer({"ABCDE"}), settings);

        REQUIRE_FALSE(terminal->isAlternateScreenActive());
        REQUIRE_FALSE(backend->_isAlternateScreenActive);
        REQUIRE_EQUAL(backend->_alternateScreenBufferChanges.size(), std::size_t{0});
        requireNotContains(backend->output(), "\x1b[?1049h");
    }

    void testBackBufferUsesPartialUpdatesForSmallDifferences() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});
        terminal->setBackBufferEnabled(true);

        terminal->updateScreen(createBuffer({"ABCDE"}));
        backend->clearOutput();

        terminal->updateScreen(createBuffer({"ABZDE"}));

        const auto output = backend->output();
        requireContains(output, "\x1b[H");
        requireContains(output, "\x1b[1;3HZ");
        requireNotContains(output, "ABZDE\n");
        requireNotContains(output, "\x1b[2J");
    }

    void testBackBufferRewritesTheFullFrameForLargeDifferences() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{4, 2});
        terminal->setBackBufferEnabled(true);

        terminal->updateScreen(createBuffer({
            "ABCD",
            "EFGH",
        }));
        backend->clearOutput();

        terminal->updateScreen(createBuffer({
            "WXYZ",
            "IJKL",
        }));

        const auto output = backend->output();
        requireContains(output, "WXYZ");
        requireContains(output, "\x1b[2;1HIJKL");
        requireNotContains(output, "WXYZ\nIJKL\n");
    }

    void testTerminalCanBeInitializedAndRestoredMultipleTimes() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorVisibilityCodes = false;
        auto terminal = createTerminal(backend, block::Size{4, 1});

        terminal->initializeScreen();
        terminal->write("A"_el);
        terminal->flush();
        terminal->restoreScreen();

        terminal->initializeScreen();
        terminal->write("B"_el);
        terminal->flush();
        terminal->restoreScreen();

        REQUIRE_EQUAL(backend->_initializePlatformCallCount, 2);
        REQUIRE_EQUAL(backend->_restorePlatformCallCount, 2);
        REQUIRE_EQUAL(backend->_cursorVisibilityChanges, std::vector<bool>({false, true, false, true}));
        requireContains(backend->output(), "A");
        requireContains(backend->output(), "B");
    }

    void testRestoreScreenClearsAlternateScreenStateForTheNextSession() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = createTerminal(backend, block::Size{5, 1});

        terminal->updateScreen(createBuffer({"ABCDE"}));
        REQUIRE(terminal->isAlternateScreenActive());

        terminal->restoreScreen();

        REQUIRE_FALSE(terminal->isAlternateScreenActive());
        REQUIRE_EQUAL(backend->_alternateScreenBufferChanges, std::vector<bool>({true, false}));

        backend->clearRecordedOperations();
        terminal->initializeScreen();
        terminal->updateScreen(createBuffer({"VWXYZ"}));

        REQUIRE_EQUAL(backend->_alternateScreenBufferChanges, std::vector<bool>({true}));
        requireContains(backend->output(), "\x1b[?1049h");
    }

    void testRestoreScreenResetsTheColorAndRestoresTheBackend() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsCursorVisibilityCodes = false;
        auto terminal = createTerminal(backend);

        terminal->initializeScreen();
        terminal->setColor(Color{fg::Red, bg::Blue});
        terminal->flush();
        backend->clearOutput();

        terminal->restoreScreen();

        REQUIRE_EQUAL(backend->output(), std::string{"\x1b[39;49m"});
        REQUIRE_EQUAL(backend->_restorePlatformCallCount, 1);
        REQUIRE_EQUAL(backend->_cursorVisibilityChanges.size(), std::size_t{2});
        REQUIRE_FALSE(backend->_cursorVisibilityChanges[0]);
        REQUIRE(backend->_cursorVisibilityChanges[1]);
    }

private:
    void requireContains(const std::string &text, const std::string_view expected) {
        const auto position = text.find(expected);
        REQUIRE_NOT_EQUAL(position, std::string::npos);
    }

    void requireNotContains(const std::string &text, const std::string_view unexpected) {
        const auto position = text.find(unexpected);
        REQUIRE_EQUAL(position, std::string::npos);
    }
};
