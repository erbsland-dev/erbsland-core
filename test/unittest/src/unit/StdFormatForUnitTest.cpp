// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteOffset.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unit/IntegerAmount.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unit/Version.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <ratio>
#include <string>

using namespace el::unit;

namespace erbsland::test {

struct SampleUnitTag {};

}

TESTED_TARGETS(StdFormatForUnit)
class StdFormatForUnitTest final : public el::UnitTest {
public:
    void testIntegerAmountFormatting() {
        using SampleMilliseconds = el::unit::IntegerAmount<el::test::SampleUnitTag, std::milli>;

        const auto negative = std::format("{}", SampleMilliseconds{-42});
        const auto positive = std::format("{:+}", SampleMilliseconds{42});
        REQUIRE_EQUAL(negative, std::string{"-42"});
        REQUIRE_EQUAL(positive, std::string{"+42"});
    }

    void testIntegerUnitFormatting() {

        const auto index = std::format("{}", ByteIndex{12U});
        const auto length = std::format("{:04}", ByteLength{7U});
        const auto offset = std::format("{:+}", ByteOffset{3});
        REQUIRE_EQUAL(index, std::string{"12"});
        REQUIRE_EQUAL(length, std::string{"0007"});
        REQUIRE_EQUAL(offset, std::string{"+3"});
    }

    void testIntegerUnitRangeFormatting() {

        const auto range = ByteRange{ByteIndex{2U}, ByteLength{5U}};
        const auto formatted = std::format("{}", range);
        const auto aligned = std::format("{:>6}", range);
        REQUIRE_EQUAL(formatted, std::string{"2:5"});
        REQUIRE_EQUAL(aligned, std::string{"   2:5"});
    }

    void testExitCodeFormatting() {

        const auto successful = std::format("{}", ExitCode{7});
        const auto failed = std::format("{:+}", ExitCode{-3});
        REQUIRE_EQUAL(successful, std::string{"7"});
        REQUIRE_EQUAL(failed, std::string{"-3"});
    }

    void testVersionFormatting() {

        const auto version = Version{1, 2, 3, 4};
        const auto formatted = std::format("{}", version);
        const auto aligned = std::format("{:>8}", version);
        REQUIRE_EQUAL(formatted, std::string{"1.2.3"});
        REQUIRE_EQUAL(aligned, std::string{"   1.2.3"});
    }
};
