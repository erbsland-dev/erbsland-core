// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/ResultWithData.hpp>

#include <string>

TESTED_TARGETS(ResultWithData)
class ResultWithDataTest final : public el::UnitTest {
public:
    void testSuccessAndFailureKeepData() {
        auto success = el::util::ResultWithData<std::string>{el::util::Result::Success, "value"};
        auto failure = el::util::ResultWithData<int>{el::util::Result::Failure, 42};

        REQUIRE(isSuccessful(success));
        REQUIRE_EQUAL(success.data(), std::string{"value"});
        REQUIRE(isFailure(failure));
        REQUIRE_EQUAL(failure.data(), 42);
    }

    void testTakeData() {
        auto result = el::util::ResultWithData<std::string>{el::util::Result::Success, "moved"};
        REQUIRE_EQUAL(result.takeData(), std::string{"moved"});
    }
};
