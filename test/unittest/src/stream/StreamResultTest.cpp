// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/StreamCloseStatus.hpp>
#include <erbsland/stream/StreamReadResult.hpp>
#include <erbsland/stream/StreamWaitStatus.hpp>
#include <erbsland/stream/StreamWriteStatus.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>

TESTED_TARGETS(StreamReadStatus StreamWriteStatus StreamCloseStatus StreamWaitStatus StreamReadResult)
class StreamResultTest final : public el::UnitTest {
public:
    void testSuccessAndFailureSemantics() {
        REQUIRE(isSuccessful(el::stream::StreamReadStatus::Data));
        REQUIRE(isSuccessful(el::stream::StreamReadStatus::Finished));
        REQUIRE(isFailure(el::stream::StreamReadStatus::Timeout));
        REQUIRE(isSuccessful(el::stream::StreamWriteStatus::Success));
        REQUIRE(isFailure(el::stream::StreamWriteStatus::Timeout));
        REQUIRE(isSuccessful(el::stream::StreamCloseStatus::Closed));
        REQUIRE(isFailure(el::stream::StreamCloseStatus::Timeout));
        REQUIRE(isSuccessful(el::stream::StreamWaitStatus::Ready));
        REQUIRE(isFailure(el::stream::StreamWaitStatus::Timeout));
    }

    void testReadResultTransportsDataAndStatus() {
        const auto result = el::stream::StreamReadResult<std::string>{el::stream::StreamReadStatus::Data, "text"};

        REQUIRE_EQUAL(result, el::stream::StreamReadStatus::Data);
        REQUIRE(result.hasData());
        REQUIRE_EQUAL(result.status(), el::stream::StreamReadStatus::Data);
        REQUIRE(isSuccessful(result));
        REQUIRE_EQUAL(result.data(), std::string{"text"});

        static_assert(std::is_base_of_v<el::stream::StreamReadStatus, decltype(result)>);
        static_assert(!std::is_constructible_v<decltype(result), el::stream::StreamWriteStatus, std::string>);
    }
};
