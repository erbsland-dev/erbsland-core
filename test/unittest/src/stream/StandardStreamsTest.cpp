// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/AnyStringBuilderStream.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/stream/TextInputStream.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using el::stream::stdErr;
using el::stream::stdIn;
using el::stream::stdOut;
using namespace el::text::literals;

namespace standard_streams_io = el::stream::io;

static_assert(requires { standard_streams_io::print("value=", 1, true); });
static_assert(requires { standard_streams_io::printLine("value=", 1, true); });
static_assert(requires { standard_streams_io::printError("value=", 1, true); });
static_assert(requires { standard_streams_io::printErrorLine("value=", 1, true); });

TESTED_TARGETS(stdIn stdOut stdErr)
class StandardStreamsTest final : public el::UnitTest {
    class MemoryTextInputStream final : public el::stream::TextInputStream {
    public:
        explicit MemoryTextInputStream(std::string text) : _text{std::move(text)} {}

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
            const auto text = read(el::unit::CpLength{1U});
            if (text != el::stream::StreamReadStatus::Data || text.data().isEmpty()) {
                return {text, el::text::Char{}};
            }
            return {el::stream::StreamReadStatus::Data, text.data()[el::unit::CpIndex{0U}]};
        }

        [[nodiscard]] auto read(el::unit::CpLength maximum) -> el::stream::StreamReadResult<el::text::String> override {
            if (_position >= _text.size()) {
                return {el::stream::StreamReadStatus::Finished, el::text::String{}};
            }
            const auto count = std::min(maximum.toSizeT(), _text.size() - _position);
            auto result = _text.substr(_position, count);
            _position += count;
            return {el::stream::StreamReadStatus::Data, el::text::StringEditor{std::string_view{result}}};
        }

        [[nodiscard]] auto readLine(el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            return read(maximum);
        }

        [[nodiscard]] auto readAll(el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            return read(maximum);
        }

    public:
        using TextInputStream::read;
        using TextInputStream::readAll;
        using TextInputStream::readLine;

    private:
        std::string _text;
        std::size_t _position{0};
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

public:
    void testCachedStreams() {
        const auto firstInput = stdIn();
        const auto secondInput = stdIn();
        const auto firstOutput = stdOut();
        const auto secondOutput = stdOut();
        const auto firstError = stdErr();
        const auto secondError = stdErr();

        REQUIRE_EQUAL(firstInput, secondInput);
        REQUIRE_EQUAL(firstOutput, secondOutput);
        REQUIRE_EQUAL(firstError, secondError);
        REQUIRE_NOT_EQUAL(firstOutput, firstError);
    }

    void testCloseIsIgnored() {
        const auto input = stdIn();
        input->close();

        REQUIRE(input->isOpen());
        REQUIRE_EQUAL(input, stdIn());

        const auto output = stdOut();
        output->close();

        REQUIRE(output->isOpen());
        REQUIRE_EQUAL(output, stdOut());

        const auto error = stdErr();
        error->close();

        REQUIRE(error->isOpen());
        REQUIRE_EQUAL(error, stdErr());
    }

    void testRedirectedOutputUsesStableProxy() {
        const auto capturedOutput = stdOut();
        const auto replacement = el::stream::AnyStringBuilderStream::create();
        auto redirect = el::stream::redirectStdOut(replacement);

        REQUIRE(redirect.isActive());
        REQUIRE_EQUAL(capturedOutput, stdOut());

        capturedOutput->writeLine("captured"_el);
        stdOut()->writeLine("current"_el);

        REQUIRE_EQUAL(toStdString(replacement), std::string{"captured\ncurrent\n"});
        redirect.reset();
        REQUIRE_FALSE(redirect.isActive());
    }

    void testRedirectedInputUsesStableProxy() {
        const auto capturedInput = stdIn();
        const auto replacement = std::make_shared<MemoryTextInputStream>("captured");
        auto redirect = el::stream::redirectStdIn(replacement);

        REQUIRE(redirect.isActive());
        REQUIRE_EQUAL(capturedInput, stdIn());
        REQUIRE_EQUAL(toStdString(capturedInput->readAll(el::unit::CpLength{8U}).data()), std::string{"captured"});

        redirect.reset();
        REQUIRE_FALSE(redirect.isActive());
    }

    void testNestedRedirectsRestorePreviousTargets() {
        const auto first = el::stream::AnyStringBuilderStream::create();
        const auto second = el::stream::AnyStringBuilderStream::create();

        auto firstRedirect = el::stream::redirectStdOut(first);
        stdOut()->writeLine("first-a"_el);
        {
            auto secondRedirect = el::stream::redirectStdOut(second);
            stdOut()->writeLine("second"_el);
            REQUIRE(secondRedirect.isActive());
        }
        stdOut()->writeLine("first-b"_el);

        REQUIRE_EQUAL(toStdString(first), std::string{"first-a\nfirst-b\n"});
        REQUIRE_EQUAL(toStdString(second), std::string{"second\n"});
        REQUIRE(firstRedirect.isActive());
    }

    void testRedirectBothStreamsAndMoveGuard() {
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto error = el::stream::AnyStringBuilderStream::create();

        auto redirect = el::stream::redirectStandardStreams(output, error);
        auto movedRedirect = std::move(redirect);

        REQUIRE_FALSE(redirect.isActive());
        REQUIRE(movedRedirect.isActive());

        stdOut()->writeLine("output"_el);
        stdErr()->writeLine("error"_el);

        REQUIRE_EQUAL(toStdString(output), std::string{"output\n"});
        REQUIRE_EQUAL(toStdString(error), std::string{"error\n"});
    }

private:
    [[nodiscard]] static auto toStdString(const el::stream::AnyStringBuilderStreamPtr &stream) -> std::string {
        return el::text::StringConverter{stream->toU8String()}.toStdString();
    }

    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return el::text::StringConverter{text}.toStdString();
    }
};
