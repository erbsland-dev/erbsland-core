// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/stream/StringBuilderStream.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <utility>

using el::stream::stdErr;
using el::stream::stdOut;
using namespace el::text::literals;

namespace standard_streams_io = el::stream::io;

static_assert(requires { standard_streams_io::print("value=", 1, true); });
static_assert(requires { standard_streams_io::printLine("value=", 1, true); });
static_assert(requires { standard_streams_io::printError("value=", 1, true); });
static_assert(requires { standard_streams_io::printErrorLine("value=", 1, true); });

TESTED_TARGETS(stdOut stdErr)
class StandardStreamsTest final : public el::UnitTest {
public:
    void testCachedStreams() {
        const auto firstOutput = stdOut();
        const auto secondOutput = stdOut();
        const auto firstError = stdErr();
        const auto secondError = stdErr();

        REQUIRE_EQUAL(firstOutput, secondOutput);
        REQUIRE_EQUAL(firstError, secondError);
        REQUIRE_NOT_EQUAL(firstOutput, firstError);
    }

    void testCloseIsIgnored() {
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
        const auto replacement = el::stream::StringBuilderStream::create();
        auto redirect = el::stream::redirectStdOut(replacement);

        REQUIRE(redirect.isActive());
        REQUIRE_EQUAL(capturedOutput, stdOut());

        capturedOutput->writeLine("captured"_el);
        stdOut()->writeLine("current"_el);

        REQUIRE_EQUAL(toStdString(replacement), std::string{"captured\ncurrent\n"});
        redirect.reset();
        REQUIRE_FALSE(redirect.isActive());
    }

    void testNestedRedirectsRestorePreviousTargets() {
        const auto first = el::stream::StringBuilderStream::create();
        const auto second = el::stream::StringBuilderStream::create();

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
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();

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
    [[nodiscard]] static auto toStdString(const el::stream::StringBuilderStreamPtr &stream) -> std::string {
        return el::text::StringConverter{stream->toU8String()}.toStdString();
    }
};
