// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestBackend.hpp"

#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace el::cterm;
using namespace el::text::literals;

TESTED_TARGETS(TerminalStream TerminalOutputGuard)
class TerminalStreamTest final : public el::UnitTest {
public:
    void testWriteMethodsUseTerminal() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend);
        auto stream = TerminalStream{terminal};

        REQUIRE_EQUAL(stream.encoding(), el::text::StringEncoding::Utf8);
        REQUIRE_EQUAL(stream.effectiveEncoding(), el::text::StringEncoding::Utf8);
        REQUIRE(stream.isOpen());

        stream.write("Hello"_el);
        stream.write(el::text::Char{U' '});
        stream.writeLine("World"_el);
        stream.writeLine();
        stream.flush();

        REQUIRE_EQUAL(backend->output(), std::string{"Hello World\n\n"});
        REQUIRE_EQUAL(backend->_emitFlushCallCount, 1);
    }

    void testStyleIsAppliedAndReset() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        const auto terminal = std::make_shared<Terminal>(backend);

        auto stream = TerminalStream{terminal, BlockStyle{fg::BrightRed}};
        stream.writeLine("styled"_el);
        stream.flush();

        REQUIRE_EQUAL(backend->output(), std::string{"styled\n"});
        REQUIRE_EQUAL(backend->_emittedColors.size(), std::size_t{2});
        REQUIRE_EQUAL(backend->_emittedColors.front(), (Color{fg::BrightRed, bg::Default}));
        REQUIRE_EQUAL(backend->_emittedColors.back(), Color::reset());
    }

    void testStandardStreamsUseTerminalSynchronization() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        backend->_supportedBlockAttributeCodes = BlockAttributes{};
        const auto terminal = std::make_shared<Terminal>(backend);
        const auto [output, error] = TerminalStream::createStandardStreams(terminal);

        output->writeLine("output"_el);
        error->writeLine("error"_el);
        output->flush();
        error->flush();

        const auto text = backend->output();
        const auto outputFirst = text == std::string{"output\nerror\n"};
        const auto errorFirst = text == std::string{"error\noutput\n"};
        REQUIRE(outputFirst || errorFirst);
    }

    void testOutputGuardCanBeNested() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend);

        auto outerGuard = terminal->synchronizeOutput();
        terminal->write("outer "_el);
        {
            auto innerGuard = terminal->synchronizeOutput();
            terminal->write("inner"_el);
        }
        terminal->writeLineBreak();

        REQUIRE_EQUAL(backend->output(), std::string{"outer inner\n"});
    }

    void testDirectOutputGuardBlocksCompetingTerminalStream() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend);
        const auto stream = TerminalStream::create(terminal);
        {
            auto guard = terminal->synchronizeOutput();
            terminal->write("direct"_el);
            terminal->flush();
            stream->writeLine("stream"_el);
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            REQUIRE_EQUAL(backend->output(), std::string{"direct"});
        }
        stream->flush();

        REQUIRE_EQUAL(backend->output(), std::string{"directstream\n"});
    }

    void testCompetingTerminalStreamsKeepCompleteLinesAtomic() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend);
        const auto first = TerminalStream::create(terminal);
        const auto second = TerminalStream::create(terminal);
        auto threads = std::vector<std::thread>{};
        threads.emplace_back([first]() -> void {
            for (auto index = 0; index < 50; ++index)
                first->writeLine("AAAA"_el);
        });
        threads.emplace_back([second]() -> void {
            for (auto index = 0; index < 50; ++index)
                second->writeLine("BBBB"_el);
        });
        for (auto &thread : threads)
            thread.join();
        first->flush();
        second->flush();

        auto offset = std::size_t{};
        const auto output = backend->output();
        for (auto index = 0; index < 100; ++index) {
            const auto line = output.substr(offset, 5U);
            REQUIRE(line == "AAAA\n" || line == "BBBB\n");
            offset += 5U;
        }
        REQUIRE_EQUAL(offset, output.size());
    }

    void testMissingTerminalThrows() {
        auto stream = TerminalStream{TerminalPtr{}};

        REQUIRE_FALSE(stream.isOpen());
        REQUIRE_THROWS_AS(el::stream::StreamError, stream.writeLine("fail"_el));
    }
};
