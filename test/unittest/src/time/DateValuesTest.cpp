// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDataTestBase.hpp"

#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/FileHelper.hpp>

#include <format>
#include <string>

namespace fh = erbsland::unittest::fh;
using namespace el::text::literals;
using namespace el::time;

TESTED_TARGETS(Date)
class DateValuesTest final : public UNITTEST_SUBCLASS(TimeDataTestBase) {
public:
    auto additionalErrorMessages() noexcept -> std::string override { return _context; }

    void testEpochValues() {
        const auto path = std::string_view{"data/time/date_epoch.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 6, path, lineNumber);
            const auto days = Days{parseInt64(fields[0])};
            const auto dateFields = parseDateFields(fields, 1);
            const auto dayOfYear = parseInt(fields[4]);
            const auto dayOfWeek = parseInt(fields[5]);
            _context = std::format("date epoch line={} days={}", lineNumber, days.toRawValue());
            const auto date = dateFromFields(dateFields);
            REQUIRE(date.isValid());
            REQUIRE_EQUAL(date.toDaysSinceEpoch(), days);
            REQUIRE_EQUAL(Date::fromDaysSinceEpoch(days), date);
            REQUIRE_EQUAL(date.year().toRawValue(), dateFields.year);
            REQUIRE_EQUAL(date.month().toRawValue(), dateFields.month);
            REQUIRE_EQUAL(date.day().toRawValue(), dateFields.day);
            REQUIRE_EQUAL(date.dayOfYear().toRawValue(), dayOfYear);
            REQUIRE_EQUAL(date.dayOfWeek().toRawValue(), dayOfWeek);
            const auto parts = date.parts();
            REQUIRE_EQUAL(parts.year.toRawValue(), dateFields.year);
            REQUIRE_EQUAL(parts.month.toRawValue(), dateFields.month);
            REQUIRE_EQUAL(parts.day.toRawValue(), dateFields.day);
        }
    }

    void testManipulation() {
        const auto path = std::string_view{"data/time/date_manipulation.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 8, path, lineNumber);
            const auto base = dateFromFields(parseDateFields(fields, 0));
            const auto unit = fields[3];
            const auto amount = parseInt64(fields[4]);
            const auto expected = dateFromFields(parseDateFields(fields, 5));
            _context = std::format(
                "date manipulation line={} unit={} base={}", lineNumber, unit, base.toDaysSinceEpoch().toRawValue());
            auto result = Date{};
            auto inPlace = base;
            if (unit == "days") {
                result = base.added(Days{amount});
                inPlace.add(Days{amount});
            } else if (unit == "months") {
                result = base.added(Months{amount});
                inPlace.add(Months{amount});
            } else if (unit == "years") {
                result = base.added(Years{amount});
                inPlace.add(Years{amount});
            } else {
                throw std::runtime_error{std::format("{}:{}: unknown unit {}", path, lineNumber, unit)};
            }
            REQUIRE_EQUAL(result, expected);
            REQUIRE_EQUAL(inPlace, expected);
        }
    }

    void testDaysTo() {
        const auto path = std::string_view{"data/time/date_days_to.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 7, path, lineNumber);
            const auto source = dateFromFields(parseDateFields(fields, 0));
            const auto target = dateFromFields(parseDateFields(fields, 3));
            const auto expected = Days{parseInt64(fields[6])};
            _context = std::format(
                "daysTo line={} source={} target={}",
                lineNumber,
                source.toDaysSinceEpoch().toRawValue(),
                target.toDaysSinceEpoch().toRawValue());
            REQUIRE_EQUAL(source.daysTo(target), expected);
        }
    }

    void testComparison() {
        const auto path = std::string_view{"data/time/date_comparison.txt"};
        const auto lines = fh::readDataLines(path);
        auto values = std::vector<std::tuple<Date, int>>{};
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 5, path, lineNumber);
            const auto valid = parseInt(fields[0]) != 0;
            auto date = Date{};
            if (valid) {
                date = dateFromFields(parseDateFields(fields, 1));
            }
            values.emplace_back(date, parseInt(fields[4]));
        }
        requireComparisonRows(values, _context, "Date");
    }

    void testStringConversion() {
        REQUIRE_EQUAL(Date::fromYearMonthDay(2026, 7, 4).toString(), "2026-07-04"_el);
        REQUIRE(Date{}.toString().isEmpty());
    }

private:
    std::string _context;
};
