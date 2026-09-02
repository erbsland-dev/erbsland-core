// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BlockStringTestHelper.hpp"

#include <erbsland/cterm/ReadLineOptions.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(ReadLineOptions)
class ReadLineOptionsTest final : public UNITTEST_SUBCLASS(BlockStringTestHelper) {
public:
    void testDefaults() {
        const auto options = ReadLineOptions{};

        REQUIRE_EQUAL(options.displayStyle(), ReadLineDisplayStyle::HorizontalFrame);
        REQUIRE_EQUAL(options.frameBorder(), FrameBorder{FrameStyle::Light});
        REQUIRE_EQUAL(options.padding(), block::MarginPair(1));
        REQUIRE(options.title().isEmpty());
        REQUIRE_EQUAL(render(options.prompt()), std::string{"› "});
        REQUIRE(options.placeholder().isEmpty());
        REQUIRE_EQUAL(options.timeout(), erbsland::time::Seconds{0});
        REQUIRE_EQUAL(options.timeoutDisplayThreshold(), erbsland::time::Seconds{20});
        REQUIRE_EQUAL(options.maximumLength(), erbsland::unit::CpLength{4096U});
        REQUIRE_EQUAL(options.maximumLines(), erbsland::unit::LineCount{1U});
        REQUIRE_EQUAL(options.maximumDisplayLines(), erbsland::unit::LineCount{5U});
        REQUIRE_EQUAL(options.blinkInterval(), erbsland::time::Milliseconds{800});
        REQUIRE(options.history().isEmpty());
        REQUIRE(options.currentText().isEmpty());
        REQUIRE_EQUAL(options.cursorBlock(), U'█');
        REQUIRE(options.cursorStyle().attributes().isReverse());
        REQUIRE_EQUAL(options.commitKey(), Key::Enter);
        REQUIRE_EQUAL(options.newLineKey(), Key::F2);
        REQUIRE_EQUAL(options.cancelKey(), Key::Escape);
        REQUIRE(options.cleanupEnabled());
    }

    void testFluentSetters() {
        auto options = ReadLineOptions{};

        auto &result = options.setBackgroundStyle(BlockStyle{fg::White, bg::Blue})
                           .setTitleStyle(BlockStyle{fg::Yellow})
                           .setPromptStyle(BlockStyle{fg::Cyan})
                           .setPlaceholderStyle(BlockStyle{fg::BrightBlack})
                           .setTextStyle(BlockStyle{fg::Green})
                           .setCursorStyle(BlockStyle{BlockAttributes::Underline})
                           .setDisplayStyle(ReadLineDisplayStyle::Frame)
                           .setPadding(block::MarginPair{-2, 3})
                           .setTitle("Title"_el)
                           .setPrompt(BlockString{"# "_el})
                           .setPlaceholder("Value"_el)
                           .setMaximumLength(erbsland::unit::CpLength{27U})
                           .setMaximumLines(erbsland::unit::LineCount{3U})
                           .setMaximumDisplayLines(erbsland::unit::LineCount{2U})
                           .setHistory(erbsland::text::StringList{"one"_el, "two"_el})
                           .setCurrentText("draft"_el)
                           .setTimeout(erbsland::time::Seconds{12})
                           .setTimeoutDisplayThreshold(erbsland::time::Seconds{7})
                           .setBlinkInterval(erbsland::time::Milliseconds{250})
                           .setCursorBlock(Block{U'_'})
                           .setCommitKey(Key::F5)
                           .setNewLineKey(Key::F6)
                           .setCancelKey(Key::F7)
                           .setCleanupEnabled(false);

        REQUIRE_EQUAL(&result, &options);
        REQUIRE_EQUAL(options.displayStyle(), ReadLineDisplayStyle::Frame);
        REQUIRE_EQUAL(options.padding(), block::MarginPair(0, 3));
        REQUIRE_EQUAL(render(options.title()), std::string{"Title"});
        REQUIRE_EQUAL(render(options.prompt()), std::string{"# "});
        REQUIRE_EQUAL(render(options.placeholder()), std::string{"Value"});
        REQUIRE_EQUAL(options.maximumLength(), erbsland::unit::CpLength{27U});
        REQUIRE_EQUAL(options.maximumLines(), erbsland::unit::LineCount{3U});
        REQUIRE_EQUAL(options.maximumDisplayLines(), erbsland::unit::LineCount{2U});
        const auto historyCount = options.history().count().toSizeT();
        REQUIRE_EQUAL(historyCount, std::size_t{2U});
        REQUIRE_EQUAL(options.currentText(), "draft"_el);
        REQUIRE_EQUAL(options.timeout(), erbsland::time::Seconds{12});
        REQUIRE_EQUAL(options.timeoutDisplayThreshold(), erbsland::time::Seconds{7});
        REQUIRE_EQUAL(options.blinkInterval(), erbsland::time::Milliseconds{250});
        REQUIRE_EQUAL(options.cursorBlock(), U'_');
        REQUIRE_EQUAL(options.commitKey(), Key::F5);
        REQUIRE_EQUAL(options.newLineKey(), Key::F6);
        REQUIRE_EQUAL(options.cancelKey(), Key::F7);
        REQUIRE_FALSE(options.cleanupEnabled());
    }

    void testValidation() {
        auto options = ReadLineOptions{};

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setMaximumLines(erbsland::unit::LineCount{0U}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setMaximumDisplayLines(erbsland::unit::LineCount{0U}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setTimeout(erbsland::time::Seconds{-1}));
        REQUIRE_THROWS_AS(
            erbsland::err::ParameterError, options.setTimeoutDisplayThreshold(erbsland::time::Seconds{-1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setBlinkInterval(erbsland::time::Milliseconds{0}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setCursorBlock(Block{U'界'}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setCursorBlock(Block{U'\n'}));
    }
};
