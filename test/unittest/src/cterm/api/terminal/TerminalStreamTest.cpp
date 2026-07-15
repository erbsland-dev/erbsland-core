// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestBackend.hpp"

#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>

using namespace el::cterm;
using namespace el::text::literals;

TESTED_TARGETS(TerminalStream TerminalStreamSynchronization)
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

    void testStandardStreamsShareSynchronization() {
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
        REQUIRE(text == std::string{"output\nerror\n"} || text == std::string{"error\noutput\n"});
    }

    void testMissingTerminalThrows() {
        auto stream = TerminalStream{TerminalPtr{}};

        REQUIRE_FALSE(stream.isOpen());
        REQUIRE_THROWS_AS(el::stream::StreamError, stream.writeLine("fail"_el));
    }
};
