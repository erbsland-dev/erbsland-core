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

class TimeDataTestBase : public el::UnitTest {
protected:
    struct DateFields {
        int year{};
        int month{};
        int day{};
    };

    struct TimeFields {
        int hour{};
        int minute{};
        int second{};
        int64_t nanosecond{};
    };

    struct DateTimeFields {
        DateFields date;
        TimeFields time;
    };

protected:
    [[nodiscard]] static auto shouldSkipLine(const std::string &line) -> bool {
        const auto view = trim(line);
        return view.empty() || view.front() == '#';
    }
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
    static void requireFieldCount(
        const std::vector<std::string_view> &fields, std::size_t expected, std::string_view path, std::size_t line) {
        if (fields.size() != expected) {
            throw std::runtime_error{
                std::format("{}:{}: expected {} fields, got {}", path, line, expected, fields.size())};
        }
    }
    [[nodiscard]] static auto parseInt64(std::string_view text) -> int64_t {
        auto value = int64_t{};
        parseChars(text, value);
        return value;
    }
    [[nodiscard]] static auto parseInt(std::string_view text) -> int { return static_cast<int>(parseInt64(text)); }
    [[nodiscard]] static auto parseDateFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> DateFields {
        return DateFields{
            .year = parseInt(fields[offset]),
            .month = parseInt(fields[offset + 1]),
            .day = parseInt(fields[offset + 2]),
        };
    }
    [[nodiscard]] static auto parseTimeFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> TimeFields {
        return TimeFields{
            .hour = parseInt(fields[offset]),
            .minute = parseInt(fields[offset + 1]),
            .second = parseInt(fields[offset + 2]),
            .nanosecond = parseInt64(fields[offset + 3]),
        };
    }
    [[nodiscard]] static auto parseDateTimeFields(const std::vector<std::string_view> &fields, std::size_t offset)
        -> DateTimeFields {
        return DateTimeFields{
            .date = parseDateFields(fields, offset),
            .time = parseTimeFields(fields, offset + 3),
        };
    }
    [[nodiscard]] static auto dateFromFields(DateFields fields) -> el::time::Date {
        return el::time::Date{
            el::time::Year{static_cast<int16_t>(fields.year)},
            el::time::Month{static_cast<int8_t>(fields.month)},
            el::time::Day{static_cast<int8_t>(fields.day)}};
    }
    [[nodiscard]] static auto timeFromFields(TimeFields fields) -> el::time::Time {
        return el::time::Time{
            el::time::Hour{static_cast<int8_t>(fields.hour)},
            el::time::Minute{static_cast<int8_t>(fields.minute)},
            el::time::Second{static_cast<int8_t>(fields.second)},
            el::time::Nanoseconds{fields.nanosecond}};
    }
    [[nodiscard]] static auto dateTimeFromFields(DateTimeFields fields) -> el::time::DateTime {
        return el::time::DateTime{dateFromFields(fields.date), timeFromFields(fields.time)};
    }

    template <typename tValue>
    void requireComparisonRows(
        const std::vector<std::tuple<tValue, int>> &values, std::string &context, std::string_view typeName) {
        for (std::size_t leftIndex = 0; leftIndex < values.size(); ++leftIndex) {
            for (std::size_t rightIndex = 0; rightIndex < values.size(); ++rightIndex) {
                const auto &[left, leftOrder] = values[leftIndex];
                const auto &[right, rightOrder] = values[rightIndex];
                context = std::format("{} comparison left={} right={}", typeName, leftIndex, rightIndex);
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
            }
        }
    }

private:
    [[nodiscard]] static auto isSpace(char ch) -> bool { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
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
