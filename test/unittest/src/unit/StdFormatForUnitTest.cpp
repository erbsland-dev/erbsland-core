// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteOffset.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unit/IntegerAmount.hpp>
#include <erbsland/unit/StdFormatForUnit.hpp>
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

        REQUIRE_EQUAL(std::format("{}", SampleMilliseconds{-42}), std::string{"-42"});
        REQUIRE_EQUAL(std::format("{:+}", SampleMilliseconds{42}), std::string{"+42"});
    }

    void testIntegerUnitFormatting() {

        REQUIRE_EQUAL(std::format("{}", ByteIndex{12U}), std::string{"12"});
        REQUIRE_EQUAL(std::format("{:04}", ByteLength{7U}), std::string{"0007"});
        REQUIRE_EQUAL(std::format("{:+}", ByteOffset{3}), std::string{"+3"});
    }

    void testIntegerUnitRangeFormatting() {

        REQUIRE_EQUAL(std::format("{}", ByteRange{ByteIndex{2U}, ByteLength{5U}}), std::string{"2:5"});
        REQUIRE_EQUAL(std::format("{:>6}", ByteRange{ByteIndex{2U}, ByteLength{5U}}), std::string{"   2:5"});
    }

    void testExitCodeFormatting() {

        REQUIRE_EQUAL(std::format("{}", ExitCode{7}), std::string{"7"});
        REQUIRE_EQUAL(std::format("{:+}", ExitCode{-3}), std::string{"-3"});
    }

    void testVersionFormatting() {

        REQUIRE_EQUAL(std::format("{}", Version{1, 2, 3, 4}), std::string{"1.2.3"});
        REQUIRE_EQUAL(std::format("{:>8}", Version{1, 2, 3, 4}), std::string{"   1.2.3"});
    }
};
