// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::stream::stdErr;
using el::stream::stdOut;

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
};
