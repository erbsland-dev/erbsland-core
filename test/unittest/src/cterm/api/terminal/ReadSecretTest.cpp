// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestBackend.hpp"

#include <erbsland/cterm/ReadSecret.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>

using namespace erbsland::text::literals;

TESTED_TARGETS(ReadSecret)
class ReadSecretTest final : public erbsland::UnitTest {
private:
    struct TestTerminal final {
        std::shared_ptr<TerminalTestBackend> backend;
        TerminalPtr terminal;
    };

    [[nodiscard]] static auto createTerminal() -> TestTerminal {
        auto backend = std::make_shared<TerminalTestBackend>();
        auto terminal = std::make_shared<Terminal>(backend, block::Size{30, 25});
        return {std::move(backend), std::move(terminal)};
    }

public:
    void testRejectsHistoryAndCurrentText() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setHistory(erbsland::text::StringList{"old"_el});
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, ReadSecret::create(testTerminal.terminal, std::move(options)));

        options = ReadLineOptions{};
        options.setCurrentText("draft"_el);
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, ReadSecret::create(testTerminal.terminal, std::move(options)));
    }

    void testProtectedUnicodeEditingAndMasking() {
        auto testTerminal = createTerminal();
        testTerminal.backend->_readKeyResults.push(Key{U'a'});
        testTerminal.backend->_readKeyResults.push(Key{U'b'});
        testTerminal.backend->_readKeyResults.push(Key::Left);
        testTerminal.backend->_readKeyResults.push(Key{Key::Combined, erbsland::text::U32String{U"e\u0301"_el}});
        testTerminal.backend->_readKeyResults.push(Key::Enter);

        const auto editor = ReadSecret::create(testTerminal.terminal);
        const auto result = editor->waitForInput();
        const auto expectedText =
            erbsland::text::StringConverter{erbsland::text::U32String{U"ae\u0301b"_el}}.toString();
        const auto expected = erbsland::text::String{expectedText};
        const auto actual = result.data();
        const auto output = testTerminal.backend->output();
        const auto visibleAePosition = output.find("ae");
        const auto visibleBPosition = output.find("b");

        REQUIRE(result.isCommitted());
        REQUIRE_EQUAL(actual, expected);
        REQUIRE(actual.isSensitive());
        REQUIRE_EQUAL(visibleAePosition, std::string::npos);
        REQUIRE_EQUAL(visibleBPosition, std::string::npos);
        REQUIRE_FALSE(editor->isActive());
    }

    void testCursorDeletionAndCancellation() {
        auto testTerminal = createTerminal();
        auto editor = ReadSecret::create(testTerminal.terminal);
        editor->start();
        for (const auto key : {Key{U'a'}, Key{U'b'}, Key{U'c'}, Key{Key::Left}, Key{Key::Backspace}, Key{Key::Enter}}) {
            testTerminal.backend->_readKeyResults.push(key);
            const auto result = editor->update();
            if (key == Key::Enter) {
                const auto actual = result.data();
                REQUIRE(result.isCommitted());
                REQUIRE_EQUAL(actual, erbsland::text::String{"ac"});
                REQUIRE(actual.isSensitive());
            }
        }
        editor->stop();

        editor->start();
        testTerminal.backend->_readKeyResults.push(Key{U'x'});
        static_cast<void>(editor->update());
        testTerminal.backend->_readKeyResults.push(Key::Escape);
        const auto cancelled = editor->update();
        REQUIRE(cancelled.isCancelled());
        REQUIRE(cancelled.data().isEmpty());
        editor->stop();
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMaximumLengthIsCappedAt1024CodePoints() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setMaximumLength(erbsland::unit::CpLength{4096U});
        const auto editor = ReadSecret::create(testTerminal.terminal, options);
        for (auto i = std::size_t{}; i < 1025U; ++i) {
            testTerminal.backend->_readKeyResults.push(Key{U'x'});
        }
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = editor->waitForInput();
        REQUIRE(result.isCommitted());
        const auto characterLength = result.data().characterLength();
        REQUIRE_EQUAL(characterLength, erbsland::unit::CpLength{1024U});
    }

    void testMaximumLengthIsCappedAt1024CodePointsLight() {
        auto testTerminal = createTerminal();
        auto options = ReadLineOptions{};
        options.setMaximumLength(erbsland::unit::CpLength{4U});
        const auto editor = ReadSecret::create(testTerminal.terminal, options);
        for (auto i = std::size_t{}; i < 5U; ++i) {
            testTerminal.backend->_readKeyResults.push(Key{U'x'});
        }
        testTerminal.backend->_readKeyResults.push(Key::Enter);
        const auto result = editor->waitForInput();
        REQUIRE(result.isCommitted());
        REQUIRE_EQUAL(result.data().characterLength(), erbsland::unit::CpLength{4U});
    }
};
