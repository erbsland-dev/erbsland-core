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

TESTED_TARGETS(DateTime)
class DateTimeValuesTest final : public UNITTEST_SUBCLASS(TimeDataTestBase) {
public:
    auto additionalErrorMessages() noexcept -> std::string override { return _context; }

    void testEpochValues() {
        const auto path = std::string_view{"data/time/datetime_epoch.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 9, path, lineNumber);
            const auto dateTimeFields = parseDateTimeFields(fields, 0);
            const auto dateTime = dateTimeFromFields(dateTimeFields);
            const auto millisecond = Milliseconds{parseInt64(fields[7])};
            const auto secondsSinceEpoch = Seconds{parseInt64(fields[8])};
            _context = std::format("datetime epoch line={} seconds={}", lineNumber, secondsSinceEpoch.toRawValue());
            REQUIRE(dateTime.isValid());
            REQUIRE_EQUAL(dateTime.year().toRawValue(), dateTimeFields.date.year);
            REQUIRE_EQUAL(dateTime.month().toRawValue(), dateTimeFields.date.month);
            REQUIRE_EQUAL(dateTime.day().toRawValue(), dateTimeFields.date.day);
            REQUIRE_EQUAL(dateTime.hour().toRawValue(), dateTimeFields.time.hour);
            REQUIRE_EQUAL(dateTime.minute().toRawValue(), dateTimeFields.time.minute);
            REQUIRE_EQUAL(dateTime.second().toRawValue(), dateTimeFields.time.second);
            REQUIRE_EQUAL(dateTime.millisecondFraction(), millisecond);
            REQUIRE_EQUAL(dateTime.nanosecondFraction().toRawValue(), dateTimeFields.time.nanosecond);
            REQUIRE_EQUAL(dateTime.toSecondsSinceEpoch(), secondsSinceEpoch);
            const auto fromSeconds = DateTime::fromSecondsSinceEpoch(secondsSinceEpoch);
            REQUIRE(fromSeconds.isValid());
            REQUIRE_EQUAL(fromSeconds.toSecondsSinceEpoch(), secondsSinceEpoch);
            REQUIRE_EQUAL(fromSeconds.nanosecondFraction(), Nanoseconds{});
        }
    }

    void testAddDuration() {
        const auto path = std::string_view{"data/time/datetime_add_duration.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 22, path, lineNumber);
            const auto base = dateTimeFromFields(parseDateTimeFields(fields, 0));
            const auto delta = Duration{Seconds{parseInt64(fields[7])}};
            const auto addExpected = dateTimeFromFields(parseDateTimeFields(fields, 8));
            const auto subtractExpected = dateTimeFromFields(parseDateTimeFields(fields, 15));
            _context = std::format(
                "datetime add line={} base={} delta={}",
                lineNumber,
                base.toSecondsSinceEpoch().toRawValue(),
                delta.toSeconds().toRawValue());
            auto value = base.added(delta);
            REQUIRE_EQUAL(value, addExpected);
            value = base;
            value.add(delta);
            REQUIRE_EQUAL(value, addExpected);
            value = base + delta;
            REQUIRE_EQUAL(value, addExpected);
            value = base;
            value += delta;
            REQUIRE_EQUAL(value, addExpected);
            value = base.subtracted(delta);
            REQUIRE_EQUAL(value, subtractExpected);
            value = base;
            value.subtract(delta);
            REQUIRE_EQUAL(value, subtractExpected);
            value = base - delta;
            REQUIRE_EQUAL(value, subtractExpected);
            value = base;
            value -= delta;
            REQUIRE_EQUAL(value, subtractExpected);
        }
    }

    void testComparison() {
        const auto path = std::string_view{"data/time/datetime_comparison.txt"};
        const auto lines = fh::readDataLines(path);
        auto values = std::vector<std::tuple<DateTime, int>>{};
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 9, path, lineNumber);
            const auto valid = parseInt(fields[0]) != 0;
            auto dateTime = DateTime{};
            if (valid) {
                dateTime = dateTimeFromFields(parseDateTimeFields(fields, 1));
            }
            values.emplace_back(dateTime, parseInt(fields[8]));
        }
        requireComparisonRows(values, _context, "DateTime");
    }

    void testStringConversion() {
        REQUIRE(DateTime{}.toString().isEmpty());
        REQUIRE_EQUAL(
            (DateTime{Date::fromYearMonthDay(2026, 7, 4), Time{Hour{7}, Minute{4}, Second{3}}}.toString()),
            "2026-07-04 07:04:03Z"_el);
    }

private:
    std::string _context;
};
