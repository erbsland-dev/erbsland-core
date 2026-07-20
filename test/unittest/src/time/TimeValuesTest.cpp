// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDataTestBase.hpp"

#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/unittest/FileHelper.hpp>

#include <format>
#include <string>

namespace fh = erbsland::unittest::fh;
using namespace el::text::literals;
using namespace el::time;

TESTED_TARGETS(Time)
class TimeValuesTest final : public UNITTEST_SUBCLASS(TimeDataTestBase) {
public:
    auto additionalErrorMessages() noexcept -> std::string override { return _context; }

    void testMidnightValues() {
        const auto path = std::string_view{"data/time/time_midnight.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 5, path, lineNumber);
            const auto time = timeFromFields(parseTimeFields(fields, 0));
            const auto nanoseconds = Nanoseconds{parseInt64(fields[4])};
            _context = std::format("time midnight line={} ns={}", lineNumber, nanoseconds.toRawValue());
            REQUIRE_EQUAL(time.timeDeltaSinceMidnight(), TimeDelta{nanoseconds});
            REQUIRE_EQUAL(time.toNanosecondsSinceMidnight(), nanoseconds);
        }
    }

    void testComparison() {
        const auto path = std::string_view{"data/time/time_comparison.txt"};
        const auto lines = fh::readDataLines(path);
        auto values = std::vector<std::tuple<Time, int>>{};
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 5, path, lineNumber);
            values.emplace_back(timeFromFields(parseTimeFields(fields, 0)), parseInt(fields[4]));
        }
        requireComparisonRows(values, _context, "Time");
    }

    void testStringConversion() {
        REQUIRE_EQUAL(Time{}.toString(), "00:00:00"_el);
        REQUIRE_EQUAL((Time{Hour{7}, Minute{4}, Second{3}, Nanoseconds{120'340'000}}.toString()), "07:04:03.12034"_el);
    }

private:
    std::string _context;
};
