// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../cterm/support/TerminalTestBackend.hpp"

#include <erbsland/cterm/BlockStringEditor.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/log/ConsoleLogWriter.hpp>
#include <erbsland/log/LogEntry.hpp>
#include <erbsland/log/LogLine.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace el::cterm;
using namespace el::text::literals;

TESTED_TARGETS(ConsoleLogWriter ConsoleLogWriterOptions)
class ConsoleLogWriterTest final : public el::UnitTest {
public:
    void testStyledParagraphUsesTerminalWidthWrappingAndIndentation() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend, bgeo::BlockSize{6, 4});
        terminal->setOutputMode(Terminal::OutputMode::BlockText);
        auto paragraph = ParagraphOptions{};
        paragraph.setWrappedLineIndent(3);
        paragraph.setLineBreakStartMark(BlockStringEditor{">"_el});
        paragraph.setLineBreakEndMark(BlockStringEditor{"<"_el});
        auto options = el::log::ConsoleLogWriterOptions{};
        options.setParagraphOptions(paragraph);
        auto writer = el::log::ConsoleLogWriter{terminal, options};
        const auto entry = std::make_shared<el::log::LogEntry>(
            1U, el::time::DateTime::now(), el::log::LogLevel::Information, el::log::LogPath{}, "AA BB CC"_el);
        const auto line = std::make_shared<el::log::LogLine>(
            std::vector<el::log::LogLineSegment>{{el::log::LogLinePart::Message, "AA BB CC"_el}});

        writer.write(entry, line);

        REQUIRE_EQUAL(backend->output(), std::string{"AA BB<\n   >CC\n"});
        REQUIRE_EQUAL(backend->_emitFlushCallCount, 1);
    }

    void testBlockTextModeSuppressesStylesForNonInteractiveOutput() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_isInteractive = false;
        const auto terminal = std::make_shared<Terminal>(backend);
        auto writer = el::log::ConsoleLogWriter{terminal};
        const auto entry = std::make_shared<el::log::LogEntry>(
            1U, el::time::DateTime::now(), el::log::LogLevel::Error, el::log::LogPath{}, "plain"_el);

        writer.write(
            entry,
            std::make_shared<el::log::LogLine>(
                std::vector<el::log::LogLineSegment>{{el::log::LogLinePart::Message, "plain"_el}}));

        REQUIRE_EQUAL(backend->output(), std::string{"plain\n"});
        REQUIRE(backend->_emittedColors.empty());
        REQUIRE_EQUAL(terminal->outputMode(), Terminal::OutputMode::BlockText);
    }
};
