// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestBackend.hpp"
#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/impl/ReadLine.hpp>
#include <erbsland/cterm/ReadLine.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimePoint.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace erbsland::cterm::impl {

class ReadLineTestAccess final {
public:
    static auto create(TerminalPtr terminal, ReadLineOptions options, std::function<time::TimePoint()> nowFn)
        -> ReadLinePtr {
        return ReadLinePtr{new ReadLine{std::move(terminal), std::move(options), std::move(nowFn)}};
    }
};

}

TESTED_TARGETS(ReadLine)
class ReadLineTest final : public UNITTEST_SUBCLASS(TestHelper) {
private:
    struct TestTerminal final {
        std::shared_ptr<TerminalTestBackend> backend;
        TerminalPtr terminal;
    };

    [[nodiscard]] static auto createTerminal(const int width = 20) -> TestTerminal {
        auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = std::make_shared<Terminal>(backend, block::Size{width, 25});
        return {std::move(backend), std::move(terminal)};
    }

    static void enter(const TestTerminal &testTerminal, const ReadLinePtr &readLine, const Key key) {
        testTerminal.backend->_readKeyResults.push(key);
        static_cast<void>(readLine->update());
    }

    void requireOutputContains(const std::string &output, const std::string_view &text) {
        const auto position = output.find(text);
        REQUIRE_NOT_EQUAL(position, std::string::npos);
    }

    void requireOutputMissing(const std::string &output, const std::string_view &text) {
        const auto position = output.find(text);
        REQUIRE_EQUAL(position, std::string::npos);
    }

public:
    void testStatusClassificationAndPayloads() {
        const auto committed = ReadLineResult{ReadLineStatus::Committed, "text"_el};
        const auto idle = ReadLineResult{ReadLineStatus::Idle, erbsland::text::String{}};
        const auto cancelled = ReadLineResult{ReadLineStatus::Cancelled, erbsland::text::String{}};
        const auto timeout = ReadLineResult{ReadLineStatus::Timeout, erbsland::text::String{}};

        REQUIRE(committed.isSuccessful());
        REQUIRE(committed.isCommitted());
        REQUIRE_EQUAL(committed.data(), "text"_el);
        REQUIRE(idle.isFailure());
        REQUIRE(idle.isIdle());
        REQUIRE(cancelled.isFailure());
        REQUIRE(cancelled.isCancelled());
        REQUIRE(timeout.isFailure());
        REQUIRE(timeout.isTimeout());
    }

    void testLifecycleAndCancellation() {
        auto testTerminal = createTerminal();
        auto readLine = ReadLine::create(testTerminal.terminal);

        REQUIRE_FALSE(readLine->isActive());
        REQUIRE_THROWS_AS(erbsland::err::RuntimeError, readLine->update());
        readLine->start();
        REQUIRE(readLine->isActive());
        REQUIRE_EQUAL(testTerminal.backend->_inputMode, Input::Mode::Key);
        REQUIRE_THROWS_AS(erbsland::err::RuntimeError, readLine->start());
        REQUIRE(readLine->update().isIdle());

        testTerminal.backend->_readKeyResults.push(Key::Escape);
        const auto result = readLine->update();
        REQUIRE(result.isCancelled());
        REQUIRE(result.data().isEmpty());
        REQUIRE(readLine->update().isCancelled());

        readLine->stop();
        REQUIRE_FALSE(readLine->isActive());
        REQUIRE_EQUAL(testTerminal.backend->_inputMode, Input::Mode::ReadLine);
        readLine->stop();
    }

    void testConstructionAndUnsupportedTerminals() {
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, ReadLine::create(TerminalPtr{}, ReadLineOptions{}));

        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setNewLineKey(Key::Enter);
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, ReadLine::create(testTerminal.terminal, options));
        options = ReadLineOptions{};
        options.setDisplayStyle(static_cast<ReadLineDisplayStyle>(255));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, ReadLine::create(testTerminal.terminal, options));

        testTerminal.backend->_isInteractive = false;
        auto nonInteractive = ReadLine::create(testTerminal.terminal);
        REQUIRE_THROWS_AS(erbsland::err::RuntimeError, nonInteractive->start());

        testTerminal.backend->_isInteractive = true;
        testTerminal.terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto plainOutput = ReadLine::create(testTerminal.terminal);
        REQUIRE_THROWS_AS(erbsland::err::RuntimeError, plainOutput->start());
    }

    void testBlockingEditingAndUnicode() {
        auto testTerminal = createTerminal();
        testTerminal.backend->_readKeyResults.push(Key{U'a'});
        testTerminal.backend->_readKeyResults.push(Key{U'b'});
        testTerminal.backend->_readKeyResults.push(Key::Left);
        testTerminal.backend->_readKeyResults.push(Key{Key::Combined, erbsland::text::U32String{U"e\u0301"_el}});
        testTerminal.backend->_readKeyResults.push(Key::Enter);

        auto readLine = ReadLine::create(testTerminal.terminal);
        const auto result = readLine->waitForInput();

        REQUIRE(result.isCommitted());
        REQUIRE_EQUAL(
            result.data(), erbsland::text::StringConverter{erbsland::text::U32String{U"ae\u0301b"_el}}.toString());
        REQUIRE_FALSE(readLine->isActive());
        REQUIRE_EQUAL(testTerminal.backend->_inputMode, Input::Mode::ReadLine);
    }

    void testDeletionTreatsCombiningSequencesAsUnits() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setCurrentText(erbsland::text::StringConverter{erbsland::text::U32String{U"ae\u0301b"_el}}.toString());
        auto readLine = ReadLine::create(testTerminal.terminal, options);
        readLine->start();

        enter(testTerminal, readLine, Key::Left);
        enter(testTerminal, readLine, Key::Backspace);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = readLine->update();

        REQUIRE(result.isCommitted());
        REQUIRE_EQUAL(result.data(), "ab"_el);
        readLine->stop();
    }

    void testLengthAndLineLimits() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setMaximumLength(erbsland::unit::CpLength{4U});
        options.setMaximumLines(erbsland::unit::LineCount{2U});
        options.setCurrentText("ab"_el);
        auto readLine = ReadLine::create(testTerminal.terminal, options);
        readLine->start();

        enter(testTerminal, readLine, Key::F2);
        enter(testTerminal, readLine, Key{U'c'});
        enter(testTerminal, readLine, Key{U'd'});
        enter(testTerminal, readLine, Key::F2);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = readLine->update();

        REQUIRE_EQUAL(result.data(), "ab\nc"_el);
        readLine->stop();
    }

    void testSingleLineDefaultIgnoresF2() {
        auto testTerminal = createTerminal();
        testTerminal.backend->_readKeyResults.push(Key{U'a'});
        testTerminal.backend->_readKeyResults.push(Key::F2);
        testTerminal.backend->_readKeyResults.push(Key{U'b'});
        testTerminal.backend->_readKeyResults.push(Key::Enter);

        auto readLine = ReadLine::create(testTerminal.terminal);
        REQUIRE_EQUAL(readLine->waitForInput().data(), "ab"_el);
    }

    void testHistoryNavigationAndDraftRestoration() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setHistory(erbsland::text::StringList{"first"_el, "second"_el});
        options.setCurrentText("draft"_el);
        auto readLine = ReadLine::create(testTerminal.terminal, options);
        readLine->start();

        enter(testTerminal, readLine, Key::Up);
        enter(testTerminal, readLine, Key::Up);
        enter(testTerminal, readLine, Key::Down);
        enter(testTerminal, readLine, Key::Down);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = readLine->update();

        REQUIRE_EQUAL(result.data(), "draft"_el);
        readLine->stop();
    }

    void testCommittedHistoryIsReusedWithoutConsecutiveDuplicates() {
        auto testTerminal = createTerminal();
        auto readLine = ReadLine::create(testTerminal.terminal);

        testTerminal.backend->_readKeyResults.push(Key{U'x'});
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        REQUIRE_EQUAL(readLine->waitForInput().data(), "x"_el);

        testTerminal.backend->_readKeyResults.push(Key::Up);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        REQUIRE_EQUAL(readLine->waitForInput().data(), "x"_el);

        readLine->start();
        enter(testTerminal, readLine, Key::Up);
        enter(testTerminal, readLine, Key::Up);
        enter(testTerminal, readLine, Key::Down);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        REQUIRE(readLine->update().data().isEmpty());
        readLine->stop();
    }

    void testVerticalMovementUsesVisualRowsAndPreservesTheColumn() {
        auto testTerminal = createTerminal(8);
        auto options = ReadLineOptions{};
        options.setCurrentText("abcdef"_el);
        auto readLine = ReadLine::create(testTerminal.terminal, options);
        readLine->start();

        enter(testTerminal, readLine, Key::Up);
        enter(testTerminal, readLine, Key{U'X'});
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = readLine->update();

        REQUIRE_EQUAL(result.data(), "abXcdef"_el);
        readLine->stop();
    }

    void testConfiguredTextAndHistoryAreNormalizedAfterAllSetters() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setCurrentText("abcd\nef"_el);
        options.setHistory(erbsland::text::StringList{"12345"_el});
        options.setMaximumLength(erbsland::unit::CpLength{4U});
        auto readLine = ReadLine::create(testTerminal.terminal, options);
        readLine->start();

        enter(testTerminal, readLine, Key::Up);
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto historyResult = readLine->update();
        REQUIRE_EQUAL(historyResult.data(), "1234"_el);
        readLine->stop();

        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto initialResult = readLine->waitForInput();
        REQUIRE_EQUAL(initialResult.data(), "abcd"_el);
    }

    void testConfiguredControlCharactersAndLineLimitAreNormalized() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setCurrentText("a\r\nb\t\nc"_el);
        options.setMaximumLines(erbsland::unit::LineCount{2U});
        testTerminal.backend->_readKeyResults.push(Key::Enter);

        const auto result = ReadLine::create(testTerminal.terminal, options)->waitForInput();

        REQUIRE_EQUAL(result.data(), "a\nb"_el);
    }

    void testDeterministicInactivityTimeoutAndActivityReset() {
        auto now = erbsland::time::TimePoint{};
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setTimeout(erbsland::time::Seconds{3});
        auto readLine = erbsland::cterm::impl::ReadLineTestAccess::create(
            testTerminal.terminal, options, [&now]() noexcept { return now; });
        readLine->start();
        requireOutputContains(testTerminal.backend->output(), "[3s]");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::seconds(2);
        REQUIRE(readLine->update().isIdle());
        requireOutputContains(testTerminal.backend->output(), "[1s]");
        testTerminal.backend->clearOutput();
        testTerminal.backend->_readKeyResults.push(Key::Left);
        REQUIRE(readLine->update().isIdle());
        requireOutputContains(testTerminal.backend->output(), "[3s]");
        now += erbsland::time::TimeDelta::seconds(2);
        REQUIRE(readLine->update().isIdle());
        now += erbsland::time::TimeDelta::seconds(1);
        const auto result = readLine->update();

        REQUIRE(result.isTimeout());
        REQUIRE(result.data().isEmpty());
        readLine->stop();
    }

    void testTimeoutCountdownAppearsAtThresholdAndCanBeDisabled() {
        auto now = erbsland::time::TimePoint{};
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setTimeout(erbsland::time::Seconds{30});
        options.setTimeoutDisplayThreshold(erbsland::time::Seconds{20});
        auto readLine = erbsland::cterm::impl::ReadLineTestAccess::create(
            testTerminal.terminal, options, [&now]() noexcept { return now; });
        readLine->start();
        requireOutputMissing(testTerminal.backend->output(), "[30s]");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::seconds(9);
        REQUIRE(readLine->update().isIdle());
        requireOutputMissing(testTerminal.backend->output(), "[21s]");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::seconds(1);
        REQUIRE(readLine->update().isIdle());
        requireOutputContains(testTerminal.backend->output(), "[20s]");

        testTerminal.backend->clearOutput();
        testTerminal.backend->_readKeyResults.push(Key::Left);
        REQUIRE(readLine->update().isIdle());
        requireOutputMissing(testTerminal.backend->output(), "[30s]");
        readLine->stop();

        auto disabledTerminal = createTerminal();
        options.setTimeout(erbsland::time::Seconds{3});
        options.setTimeoutDisplayThreshold(erbsland::time::Seconds::zero());
        auto disabled = erbsland::cterm::impl::ReadLineTestAccess::create(
            disabledTerminal.terminal, options, [&now]() noexcept { return now; });
        disabled->start();
        requireOutputMissing(disabledTerminal.backend->output(), "[3s]");
        disabled->stop();
    }

    void testCursorBlinkUsesTheRenderedCursor() {
        auto now = erbsland::time::TimePoint{};
        auto testTerminal = createTerminal();
        auto readLine = erbsland::cterm::impl::ReadLineTestAccess::create(
            testTerminal.terminal, ReadLineOptions{}, [&now]() noexcept { return now; });
        readLine->start();
        requireOutputContains(testTerminal.backend->output(), "█");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::milliseconds(800);
        REQUIRE(readLine->update().isIdle());
        requireOutputMissing(testTerminal.backend->output(), "█");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::milliseconds(800);
        REQUIRE(readLine->update().isIdle());
        requireOutputContains(testTerminal.backend->output(), "█");
        readLine->stop();
    }

    void testCursorMovementRestartsTheVisibleBlinkPhase() {
        auto now = erbsland::time::TimePoint{};
        auto testTerminal = createTerminal();
        auto readLine = erbsland::cterm::impl::ReadLineTestAccess::create(
            testTerminal.terminal, ReadLineOptions{}, [&now]() noexcept { return now; });
        readLine->start();

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::milliseconds(800);
        REQUIRE(readLine->update().isIdle());
        requireOutputMissing(testTerminal.backend->output(), "█");

        testTerminal.backend->clearOutput();
        testTerminal.backend->_readKeyResults.push(Key{U'a'});
        REQUIRE(readLine->update().isIdle());
        requireOutputContains(testTerminal.backend->output(), "█");

        testTerminal.backend->clearOutput();
        now += erbsland::time::TimeDelta::milliseconds(800);
        REQUIRE(readLine->update().isIdle());
        requireOutputMissing(testTerminal.backend->output(), "█");
        readLine->stop();
    }

    void testRenderingLayoutsPlaceholderSecretAndResize() {
        for (
            const auto style :
            {ReadLineDisplayStyle::Compact,
                ReadLineDisplayStyle::HorizontalSpace,
                ReadLineDisplayStyle::HorizontalFrame,
                ReadLineDisplayStyle::Frame}) {
            auto testTerminal = createTerminal(16);
            auto options = ReadLineOptions{};
            testTerminal.backend->_supportsColorCodes = false;
            testTerminal.backend->_supportsCursorCodes = false;
            testTerminal.backend->_supportedBlockAttributeCodes = {};
            options.setDisplayStyle(style);
            options.setTitle("Code"_el);
            options.setPlaceholder("value"_el);
            options.setCleanupEnabled(false);
            auto readLine = ReadLine::create(testTerminal.terminal, options);
            readLine->start();

            auto expected = std::string{};
            switch (style) {
            case ReadLineDisplayStyle::Compact:
                expected = erbsland::text::StringConverter{"Code            \n"
                                                           " › value        \n"_el}
                               .toStdString();
                break;
            case ReadLineDisplayStyle::HorizontalSpace:
                expected = erbsland::text::StringConverter{"Code            \n"
                                                           " › value        \n"
                                                           "                \n"_el}
                               .toStdString();
                break;
            case ReadLineDisplayStyle::HorizontalFrame:
                expected = erbsland::text::StringConverter{"─── Code ───────\n"
                                                           " › value        \n"
                                                           "────────────────\n"_el}
                               .toStdString();
                break;
            case ReadLineDisplayStyle::Frame:
                expected = erbsland::text::StringConverter{"┌─ Code ───────┐\n"
                                                           "│ › value      │\n"
                                                           "└──────────────┘\n"_el}
                               .toStdString();
                break;
            }
            REQUIRE_EQUAL(testTerminal.backend->output(), expected);
            testTerminal.backend->clearOutput();
            enter(testTerminal, readLine, Key{U'x'});
            requireOutputContains(testTerminal.backend->output(), "x");

            testTerminal.terminal->setSize(block::Size{12, 25});
            testTerminal.backend->clearOutput();
            REQUIRE(readLine->update().isIdle());
            REQUIRE_FALSE(testTerminal.backend->output().empty());
            readLine->stop();
        }

        auto narrowTerminal = createTerminal(6);
        narrowTerminal.backend->_supportsColorCodes = false;
        narrowTerminal.backend->_supportsCursorCodes = false;
        narrowTerminal.backend->_supportedBlockAttributeCodes = {};
        auto narrowOptions = ReadLineOptions{};
        narrowOptions.setDisplayStyle(ReadLineDisplayStyle::Frame);
        narrowOptions.setTitle("Code"_el);
        auto narrowReadLine = ReadLine::create(narrowTerminal.terminal, narrowOptions);
        narrowReadLine->start();
        const auto expectedTop = erbsland::text::StringConverter{"┌─ C ┐\n"_el}.toStdString();
        REQUIRE_EQUAL(narrowTerminal.backend->output().substr(0, expectedTop.size()), expectedTop);
        narrowReadLine->stop();
    }

    void testCleanupAndDestructorRestoreMode() {
        auto cleanupTerminal = createTerminal();
        {
            auto readLine = ReadLine::create(cleanupTerminal.terminal);
            readLine->start();
            cleanupTerminal.backend->clearRecordedOperations();
        }
        REQUIRE_EQUAL(cleanupTerminal.backend->_inputMode, Input::Mode::ReadLine);
        REQUIRE_FALSE(cleanupTerminal.backend->output().empty());

        auto retainedTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setCleanupEnabled(false);
        auto readLine = ReadLine::create(retainedTerminal.terminal, options);
        readLine->start();
        retainedTerminal.backend->clearRecordedOperations();
        readLine->stop();
        REQUIRE_EQUAL(retainedTerminal.backend->_inputMode, Input::Mode::ReadLine);
        REQUIRE_FALSE(retainedTerminal.backend->output().empty());
    }
};
