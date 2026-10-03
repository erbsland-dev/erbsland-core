// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <tool/test_data/impl/CalendarDate.hpp>
#include <tool/test_data/impl/CalendarDateTime.hpp>
#include <tool/test_data/SaturatingMathTool.hpp>
#include <tool/test_data/TimeTestsTool.hpp>

#include <limits>

TESTED_TARGETS(SaturatingMathTool SaturatingMathData TimeTestsTool CalendarDate CalendarTime CalendarDateTime)
class FixtureGeneratorsTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void setUp() override {
        ProjectFixture::setUp();
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[generate_time_tests]\n[generate_saturating_math_data]\n"_el);
    }
    void testCalendarBoundariesLight() {
        using tool::test_data::impl::CalendarDate;
        using tool::test_data::impl::CalendarDateTime;
        REQUIRE_EQUAL(CalendarDate::monthDays(0, 2), 29);
        REQUIRE_EQUAL(CalendarDate::monthDays(1900, 2), 28);
        REQUIRE_EQUAL(CalendarDate::monthDays(2000, 2), 29);
        REQUIRE_EQUAL((CalendarDate{1970, 1, 1}.days()), 719528);
        REQUIRE_EQUAL(CalendarDate::fromDays(3652424).fields(), "9999 12 31"_el);
        REQUIRE_EQUAL((CalendarDate{2000, 2, 29}.addedMonths(12).fields()), "2001 2 28"_el);
        const auto base = CalendarDateTime{{2000, 2, 29}, {12, 30, 0, 123}};
        const auto minimum = el::BigInteger{std::numeric_limits<int64_t>::min()};
        REQUIRE_EQUAL(base.addedSeconds(minimum).fields(), "0 1 1 0 0 0 0"_el);
        REQUIRE_EQUAL(base.addedSeconds(-minimum).fields(), "9999 12 31 23 59 59 999999999"_el);
        REQUIRE_EQUAL(base.addedSeconds(el::BigInteger{86400}).fields(), "2000 3 1 12 30 0 123"_el);
        auto first = el::FastRandom{29839};
        auto second = el::FastRandom{29839};
        for (auto index = 0; index < 100; ++index) {
            REQUIRE_EQUAL(CalendarDate::random(first).fields(), CalendarDate::random(second).fields());
        }
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEveryGregorianDate() {
        using tool::test_data::impl::CalendarDate;
        auto expected = int64_t{};
        for (auto year = 0; year < 10000; ++year) {
            for (auto month = 1; month <= 12; ++month) {
                for (auto day = 1; day <= CalendarDate::monthDays(year, month); ++day) {
                    const auto value = CalendarDate{year, month, day};
                    REQUIRE_EQUAL(value.days(), expected);
                    REQUIRE(CalendarDate::fromDays(expected) == value);
                    ++expected;
                }
            }
        }
        REQUIRE_EQUAL(expected, 3652425);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTimeOptionsCheckAndPreservedCrLf() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[generate_time_tests]\noutput_directory: \"configured\"\n"_el);
        REQUIRE(runTime(el::StringList{"--check"_el}) != 0);
        REQUIRE_FALSE((_root / "configured"_el).info().exists());
        REQUIRE_EQUAL(runTime(el::StringList{"--output-directory"_el, "override"_el}), 0);
        REQUIRE_FALSE((_root / "configured"_el).info().exists());
        const auto name = "override/datetime_epoch.txt"_el;
        const auto text = files().readText(el::Path{name}).replacedAll("\n"_el, "\r\n"_el);
        write(name, text);
        const auto before = files().readData(el::Path{name});
        REQUIRE_EQUAL(runTime(el::StringList{"--output-directory"_el, "override"_el, "--check"_el}), 0);
        REQUIRE_EQUAL(runTime(el::StringList{"--output-directory"_el, "override"_el}), 0);
        REQUIRE_EQUAL(files().readData(el::Path{name}), before);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPreparationFailuresLeaveOutputsUntouched() {
        write("blocked/date_epoch.txt"_el, "sentinel"_el);
        (_root / "blocked/time_comparison.txt"_el).operations().createDirectoryOrThrow();
        REQUIRE(runTime(el::StringList{"--output-directory"_el, "blocked"_el}) != 0);
        REQUIRE(_errorText.contains("--output-directory"_el));
        REQUIRE_EQUAL(files().readText(el::Path{"blocked/date_epoch.txt"_el}), "sentinel"_el);
        REQUIRE(runTime(el::StringList{"--output-directory"_el, "../escape"_el}) != 0);
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[generate_time_tests]\noutput_directory: 1\n"_el);
        REQUIRE(runTime() != 0);
        REQUIRE(_errorText.contains("output_directory"_el));
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSaturatingExactBoundariesAndRepeatability() {
        const auto run = [&](const el::StringList &options) {
            return runTool<tool::test_data::SaturatingMathTool>({}, {}, "generate-saturating-math-data"_el, options);
        };
        REQUIRE_EQUAL(run(el::StringList{}), 0);
        const auto multiply = files().readText(el::Path{"test/unittest/data/saturating_math/multiply.txt"_el});
        REQUIRE(multiply.contains("u64 u64 18446744073709551615 18446744073709551615 18446744073709551615 1"_el));
        const auto divide = files().readText(el::Path{"test/unittest/data/saturating_math/divide.txt"_el});
        REQUIRE(divide.contains("i64 i64 -9223372036854775808 -1 9223372036854775807 1"_el));
        const auto modulo = files().readText(el::Path{"test/unittest/data/saturating_math/modulo.txt"_el});
        REQUIRE(modulo.contains("i64 i64 -9223372036854775808 -1 0 0"_el));
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
    }

private:
    auto runTime(const el::StringList &options = {}) -> int {
        return runTool<tool::test_data::TimeTestsTool>({}, {}, "generate-time-tests"_el, options);
    }
};
