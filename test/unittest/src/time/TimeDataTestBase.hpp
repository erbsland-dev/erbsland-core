// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/time/all.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <charconv>
#include <compare>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

/// Provide shared parsing and assertion helpers for time-data tests.
/// @notest{A base class exercised by its derived test suites.}
class TimeDataTestBase : public el::UnitTest {
protected:
    /// Store parsed calendar date fields.
    struct DateFields {
        int year{};
        int month{};
        int day{};
    };

    /// Store parsed clock-time fields.
    struct TimeFields {
        int hour{};
        int minute{};
        int second{};
        int64_t nanosecond{};
    };

    /// Store parsed date and time fields.
    struct DateTimeFields {
        DateFields date;
        TimeFields time;
    };

protected:
    /// Test whether a data-file line is blank or a comment.
    [[nodiscard]] static auto shouldSkipLine(const std::string &line) -> bool {
        const auto view = trim(line);
        return view.empty() || view.front() == '#';
    }
    /// Split a data-file line into whitespace-delimited fields.
    [[nodiscard]] static auto splitFields(const std::string &line) -> std::vector<std::string_view> {
        std::vector<std::string_view> result;
        std::string_view view{line};
        auto index = std::size_t{0};
        while (index < view.size()) {
            while (index < view.size() && isSpace(view[index])) {
                ++index;
            }
            const auto start = index;
            while (index < view.size() && !isSpace(view[index])) {
                ++index;
            }
            if (start < index) {
                result.emplace_back(view.substr(start, index - start));
            }
        }
        return result;
    }
    /// Require a data-file row to have the expected number of fields.
    static void requireFieldCount(
        const std::vector<std::string_view> &fields, std::size_t expected, std::string_view path, std::size_t line) {
        if (fields.size() != expected) {
            throw std::runtime_error{
                std::format("{}:{}: expected {} fields, got {}", path, line, expected, fields.size())};
        }
    }
    /// Parse one signed 64-bit integer.
    [[nodiscard]] static auto parseInt64(std::string_view text) -> int64_t {
        auto value = int64_t{};
        parseChars(text, value);
        return value;
    }
    /// Parse one native integer.
    [[nodiscard]] static auto parseInt(std::string_view text) -> int { return static_cast<int>(parseInt64(text)); }
    /// Parse date fields beginning at `offset`.
    [[nodiscard]] static auto parseDateFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> DateFields {
        return DateFields{
            .year = parseInt(fields[offset]),
            .month = parseInt(fields[offset + 1]),
            .day = parseInt(fields[offset + 2]),
        };
    }
    /// Parse time fields beginning at `offset`.
    [[nodiscard]] static auto parseTimeFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> TimeFields {
        return TimeFields{
            .hour = parseInt(fields[offset]),
            .minute = parseInt(fields[offset + 1]),
            .second = parseInt(fields[offset + 2]),
            .nanosecond = parseInt64(fields[offset + 3]),
        };
    }
    /// Parse date and time fields beginning at `offset`.
    [[nodiscard]] static auto parseDateTimeFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> DateTimeFields {
        return DateTimeFields{
            .date = parseDateFields(fields, offset),
            .time = parseTimeFields(fields, offset + 3),
        };
    }
    /// Convert parsed date fields to a library date.
    [[nodiscard]] static auto dateFromFields(DateFields fields) -> el::time::Date {
        return el::time::Date{
            el::time::Year{static_cast<int16_t>(fields.year)},
            el::time::Month{static_cast<int8_t>(fields.month)},
            el::time::Day{static_cast<int8_t>(fields.day)}};
    }
    /// Convert parsed time fields to a library time.
    [[nodiscard]] static auto timeFromFields(TimeFields fields) -> el::time::Time {
        return el::time::Time{
            el::time::Hour{static_cast<int8_t>(fields.hour)},
            el::time::Minute{static_cast<int8_t>(fields.minute)},
            el::time::Second{static_cast<int8_t>(fields.second)},
            el::time::Nanoseconds{fields.nanosecond}};
    }
    /// Convert parsed date and time fields to a library date-time.
    [[nodiscard]] static auto dateTimeFromFields(DateTimeFields fields) -> el::time::DateTime {
        return el::time::DateTime{dateFromFields(fields.date), timeFromFields(fields.time)};
    }

    /// Compare every pair of values against their expected ordering.
    template <typename tValue>
    void requireComparisonRows(const std::vector<std::tuple<tValue, int>> &values, std::string_view typeName) {
        for (std::size_t leftIndex = 0; leftIndex < values.size(); ++leftIndex) {
            for (std::size_t rightIndex = 0; rightIndex < values.size(); ++rightIndex) {
                const auto &[left, leftOrder] = values[leftIndex];
                const auto &[right, rightOrder] = values[rightIndex];
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void {
                        REQUIRE_EQUAL(left == right, leftOrder == rightOrder);
                        REQUIRE_EQUAL(left != right, leftOrder != rightOrder);
                        REQUIRE_EQUAL(left < right, leftOrder < rightOrder);
                        REQUIRE_EQUAL(left <= right, leftOrder <= rightOrder);
                        REQUIRE_EQUAL(left > right, leftOrder > rightOrder);
                        REQUIRE_EQUAL(left >= right, leftOrder >= rightOrder);
                        const auto ordering = left <=> right;
                        if (leftOrder < rightOrder) {
                            REQUIRE(ordering == std::strong_ordering::less);
                        } else if (leftOrder > rightOrder) {
                            REQUIRE(ordering == std::strong_ordering::greater);
                        } else {
                            REQUIRE(ordering == std::strong_ordering::equal);
                        }
                    },
                    [&]() -> std::string {
                        return std::format("{} comparison left={} right={}", typeName, leftIndex, rightIndex);
                    });
            }
        }
    }

private:
    /// Test whether `ch` is ASCII whitespace.
    [[nodiscard]] static auto isSpace(char ch) -> bool { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
    /// Remove ASCII whitespace around `line`.
    [[nodiscard]] static auto trim(const std::string &line) -> std::string_view {
        std::string_view view{line};
        while (!view.empty() && isSpace(view.front())) {
            view.remove_prefix(1);
        }
        while (!view.empty() && isSpace(view.back())) {
            view.remove_suffix(1);
        }
        return view;
    }
    /// Parse a complete integer `text` into `value`.
    template <typename T>
    static void parseChars(std::string_view text, T &value) {
        const auto *begin = text.data();
        const auto *end = begin + text.size();
        const auto [ptr, error] = std::from_chars(begin, end, value);
        if (error != std::errc{} || ptr != end) {
            throw std::runtime_error{std::format("Invalid integer value: {}", text)};
        }
    }
};
