// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <ratio>
#include <string>

using namespace el::text::literals;
using namespace el::time::literals;

namespace erbsland::test {

struct MergedFormatUnitTag {};

}

TESTED_TARGETS(Literals StdFormat)
class MergedIncludeHeadersTest final : public el::UnitTest {
public:
    void testLiterals() {
        REQUIRE_EQUAL(el::text::String{"merged"_el}, el::text::String{"merged"});
        REQUIRE_EQUAL(750_ms, el::time::Milliseconds{750});
    }

    void testStdFormat() {
        using SampleAmount = el::unit::IntegerAmount<el::test::MergedFormatUnitTag, std::ratio<1>>;

        REQUIRE_EQUAL(std::format("{}", el::bgeo::BlockSize{8, 5}), std::string{"8x5"});
        REQUIRE_EQUAL(std::format("{}", el::text::String{"merged"_el}), std::string{"merged"});
        REQUIRE_EQUAL(std::format("{}", el::time::Date::fromYearMonthDay(2026, 7, 28)), std::string{"2026-07-28"});
        REQUIRE_EQUAL(std::format("{}", SampleAmount{42}), std::string{"42"});
    }
};
