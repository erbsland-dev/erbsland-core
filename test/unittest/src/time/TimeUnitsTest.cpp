// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDataTestBase.hpp"

#include <erbsland/unittest/FileHelper.hpp>

#include <format>
#include <map>
#include <string>

namespace fh = erbsland::unittest::fh;
using namespace el::time;

TESTED_TARGETS(
    TimePartWithAmount Duration TimeDelta Nanoseconds Microseconds Milliseconds Seconds Minutes Hours Days Weeks Months
        Years)
class TimeUnitsTest final : public UNITTEST_SUBCLASS(TimeDataTestBase) {
    struct ComparisonRow {
        std::string type;
        int64_t value{};
        int order{};
    };

    struct ConversionRow {
        std::string sourceType;
        int64_t value{};
        std::string targetType;
        int64_t expected{};
        std::size_t lineNumber{};
    };

public:
    auto additionalErrorMessages() noexcept -> std::string override { return _context; }

    void testComparisons() {
        const auto rows = readComparisonRows();
        requireAmountComparison<Nanoseconds>(rows, "Nanoseconds");
        requireAmountComparison<Microseconds>(rows, "Microseconds");
        requireAmountComparison<Milliseconds>(rows, "Milliseconds");
        requireAmountComparison<Seconds>(rows, "Seconds");
        requireAmountComparison<Minutes>(rows, "Minutes");
        requireAmountComparison<Hours>(rows, "Hours");
        requireAmountComparison<Days>(rows, "Days");
        requireAmountComparison<Weeks>(rows, "Weeks");
        requireAmountComparison<Months>(rows, "Months");
        requireAmountComparison<Years>(rows, "Years");
        requirePartComparison<Second>(rows, "Second");
        requirePartComparison<Minute>(rows, "Minute");
        requirePartComparison<Hour>(rows, "Hour");
        requirePartComparison<Day>(rows, "Day");
        requirePartComparison<DayOfYear>(rows, "DayOfYear");
        requirePartComparison<Month>(rows, "Month");
        requirePartComparison<Year>(rows, "Year");
        requireTimeDeltaComparison(rows);
        requireDurationComparison(rows);
    }

    void testRegularConversions() {
        const auto path = std::string_view{"data/time/time_amount_conversion.txt"};
        const auto lines = fh::readDataLines(path);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 4, path, lineNumber);
            const auto row = ConversionRow{
                .sourceType = std::string{fields[0]},
                .value = parseInt64(fields[1]),
                .targetType = std::string{fields[2]},
                .expected = parseInt64(fields[3]),
                .lineNumber = lineNumber,
            };
            WITH_CONTEXT(dispatchConversionSource(row));
        }
    }

private:
    [[nodiscard]] auto readComparisonRows() -> std::map<std::string, std::vector<ComparisonRow>> {
        const auto path = std::string_view{"data/time/time_amount_comparison.txt"};
        const auto lines = fh::readDataLines(path);
        auto rows = std::map<std::string, std::vector<ComparisonRow>>{};
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            const auto fields = splitFields(line);
            requireFieldCount(fields, 3, path, lineNumber);
            const auto type = std::string{fields[0]};
            rows[type].push_back(
                ComparisonRow{
                    .type = type,
                    .value = parseInt64(fields[1]),
                    .order = parseInt(fields[2]),
                });
        }
        return rows;
    }

    template <typename tAmount>
    void requireAmountComparison(const std::map<std::string, std::vector<ComparisonRow>> &rows, std::string_view type) {
        const auto values = amountComparisonValues<tAmount>(rows, type);
        requireComparisonRows(values, _context, type);
    }

    template <typename tAmount>
    [[nodiscard]] auto amountComparisonValues(
        const std::map<std::string, std::vector<ComparisonRow>> &rows, std::string_view type)
        -> std::vector<std::tuple<tAmount, int>> {
        auto result = std::vector<std::tuple<tAmount, int>>{};
        const auto iterator = rows.find(std::string{type});
        if (iterator == rows.end()) {
            throw std::runtime_error{std::format("Missing comparison rows for {}", type)};
        }
        for (const auto &row : iterator->second) {
            result.emplace_back(tAmount{row.value}, row.order);
        }
        return result;
    }

    template <typename tPart>
    void requirePartComparison(const std::map<std::string, std::vector<ComparisonRow>> &rows, std::string_view type) {
        auto result = std::vector<std::tuple<tPart, int>>{};
        const auto iterator = rows.find(std::string{type});
        if (iterator == rows.end()) {
            throw std::runtime_error{std::format("Missing comparison rows for {}", type)};
        }
        for (const auto &row : iterator->second) {
            result.emplace_back(tPart{static_cast<typename tPart::Value>(row.value)}, row.order);
        }
        requireComparisonRows(result, _context, type);
    }

    void requireTimeDeltaComparison(const std::map<std::string, std::vector<ComparisonRow>> &rows) {
        auto result = std::vector<std::tuple<TimeDelta, int>>{};
        const auto iterator = rows.find("TimeDelta");
        if (iterator == rows.end()) {
            throw std::runtime_error{"Missing comparison rows for TimeDelta"};
        }
        for (const auto &row : iterator->second) {
            result.emplace_back(TimeDelta{Nanoseconds{row.value}}, row.order);
        }
        requireComparisonRows(result, _context, "TimeDelta");
    }

    void requireDurationComparison(const std::map<std::string, std::vector<ComparisonRow>> &rows) {
        auto result = std::vector<std::tuple<Duration, int>>{};
        const auto iterator = rows.find("Duration");
        if (iterator == rows.end()) {
            throw std::runtime_error{"Missing comparison rows for Duration"};
        }
        for (const auto &row : iterator->second) {
            result.emplace_back(Duration{Seconds{row.value}}, row.order);
        }
        requireComparisonRows(result, _context, "Duration");
    }

    void dispatchConversionSource(const ConversionRow &row) {
        if (row.sourceType == "Nanoseconds") {
            dispatchConversionTarget<Nanoseconds>(row);
        } else if (row.sourceType == "Microseconds") {
            dispatchConversionTarget<Microseconds>(row);
        } else if (row.sourceType == "Milliseconds") {
            dispatchConversionTarget<Milliseconds>(row);
        } else if (row.sourceType == "Seconds") {
            dispatchConversionTarget<Seconds>(row);
        } else if (row.sourceType == "Minutes") {
            dispatchConversionTarget<Minutes>(row);
        } else if (row.sourceType == "Hours") {
            dispatchConversionTarget<Hours>(row);
        } else if (row.sourceType == "Days") {
            dispatchConversionTarget<Days>(row);
        } else if (row.sourceType == "Weeks") {
            dispatchConversionTarget<Weeks>(row);
        } else {
            throw std::runtime_error{std::format("Unknown source type {}", row.sourceType)};
        }
    }

    template <typename tSource>
    void dispatchConversionTarget(const ConversionRow &row) {
        if (row.targetType == "Nanoseconds") {
            checkConversion<tSource, Nanoseconds>(row);
        } else if (row.targetType == "Microseconds") {
            checkConversion<tSource, Microseconds>(row);
        } else if (row.targetType == "Milliseconds") {
            checkConversion<tSource, Milliseconds>(row);
        } else if (row.targetType == "Seconds") {
            checkConversion<tSource, Seconds>(row);
        } else if (row.targetType == "Minutes") {
            checkConversion<tSource, Minutes>(row);
        } else if (row.targetType == "Hours") {
            checkConversion<tSource, Hours>(row);
        } else if (row.targetType == "Days") {
            checkConversion<tSource, Days>(row);
        } else if (row.targetType == "Weeks") {
            checkConversion<tSource, Weeks>(row);
        } else {
            throw std::runtime_error{std::format("Unknown target type {}", row.targetType)};
        }
    }

    template <typename tSource, typename tTarget>
    void checkConversion(const ConversionRow &row) {
        const auto source = tSource{row.value};
        const auto expected = tTarget{row.expected};
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_EQUAL(source.template converted<tTarget>(), expected); },
            [&]() -> std::string {
                return std::format(
                    "conversion line={} {} value={} to {} expected={} actual={}",
                    row.lineNumber,
                    row.sourceType,
                    row.value,
                    row.targetType,
                    expected.toRawValue(),
                    source.template converted<tTarget>().toRawValue());
            });
    }

private:
    std::string _context;
};
