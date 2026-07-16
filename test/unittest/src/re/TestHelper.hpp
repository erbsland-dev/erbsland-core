// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringHelper.hpp"

#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace re_test {

class TestHelper : public el::UnitTest {
public:
    ~TestHelper() override = default;

public:
    /// Test the comparison operators of one or between two different data types.
    ///
    /// Pass six arguments that fulfil these requirements:
    /// a1 == b1, a2 == b2, a3 == b3
    /// a1 < a2, a2 < a3, b1 < b2, b2 < b3
    ///
    /// The test always puts `a` on the left, and `b` on the right side of the operator.
    template <typename A, typename B>
    void requireAllOperators(const A &a1, const A &a2, const A &a3, const B &b1, const B &b2, const B &b3) {

        // Test ==
        REQUIRE(a1 == b1);
        REQUIRE_FALSE(a1 == b2);
        REQUIRE_FALSE(a1 == b3);
        REQUIRE_FALSE(a2 == b1);
        REQUIRE(a2 == b2);
        REQUIRE_FALSE(a2 == b3);
        REQUIRE_FALSE(a3 == b1);
        REQUIRE_FALSE(a3 == b2);
        REQUIRE(a3 == b3);

        // Test !=
        REQUIRE_FALSE(a1 != b1);
        REQUIRE(a1 != b2);
        REQUIRE(a1 != b3);
        REQUIRE(a2 != b1);
        REQUIRE_FALSE(a2 != b2);
        REQUIRE(a2 != b3);
        REQUIRE(a3 != b1);
        REQUIRE(a3 != b2);
        REQUIRE_FALSE(a3 != b3);

        // Test <
        REQUIRE_FALSE(a1 < b1);
        REQUIRE(a1 < b2);
        REQUIRE(a1 < b3);
        REQUIRE_FALSE(a2 < b1);
        REQUIRE_FALSE(a2 < b2);
        REQUIRE(a2 < b3);
        REQUIRE_FALSE(a3 < b1);
        REQUIRE_FALSE(a3 < b2);
        REQUIRE_FALSE(a3 < b3);

        // Test <=
        REQUIRE(a1 <= b1);
        REQUIRE(a1 <= b2);
        REQUIRE(a1 <= b3);
        REQUIRE_FALSE(a2 <= b1);
        REQUIRE(a2 <= b2);
        REQUIRE(a2 <= b3);
        REQUIRE_FALSE(a3 <= b1);
        REQUIRE_FALSE(a3 <= b2);
        REQUIRE(a3 <= b3);

        // Test >
        REQUIRE_FALSE(a1 > b1);
        REQUIRE_FALSE(a1 > b2);
        REQUIRE_FALSE(a1 > b3);
        REQUIRE(a2 > b1);
        REQUIRE_FALSE(a2 > b2);
        REQUIRE_FALSE(a2 > b3);
        REQUIRE(a3 > b1);
        REQUIRE(a3 > b2);
        REQUIRE_FALSE(a3 > b3);

        // Test >=
        REQUIRE(a1 >= b1);
        REQUIRE_FALSE(a1 >= b2);
        REQUIRE_FALSE(a1 >= b3);
        REQUIRE(a2 >= b1);
        REQUIRE(a2 >= b2);
        REQUIRE_FALSE(a2 >= b3);
        REQUIRE(a3 >= b1);
        REQUIRE(a3 >= b2);
        REQUIRE(a3 >= b3);

        // Test <=>
        REQUIRE((a1 <=> b1) == std::strong_ordering::equal);
        REQUIRE((a1 <=> b2) == std::strong_ordering::less);
        REQUIRE((a1 <=> b3) == std::strong_ordering::less);
        REQUIRE((a2 <=> b1) == std::strong_ordering::greater);
        REQUIRE((a2 <=> b2) == std::strong_ordering::equal);
        REQUIRE((a2 <=> b3) == std::strong_ordering::less);
        REQUIRE((a3 <=> b1) == std::strong_ordering::greater);
        REQUIRE((a3 <=> b2) == std::strong_ordering::greater);
        REQUIRE((a3 <=> b3) == std::strong_ordering::equal);
    }

    /// Test if the comparison operators correctly work between a sequence of ordered values.
    /// This will test every possible combination between the given values.
    template <typename T, std::size_t tSize>
    void requireStrictOrder(const std::array<T, tSize> &valuesInOrder) {
        for (std::size_t i = 0; i < tSize; ++i) {
            for (std::size_t j = 0; j < tSize; ++j) {
                const auto &iValue = valuesInOrder[i];
                const auto &jValue = valuesInOrder[j];
                const auto isEqual = i == j;
                const auto isLess = i < j;
                const auto isLessOrEqual = i <= j;
                const auto isGreater = i > j;
                const auto isGreaterOrEqual = i >= j;
                REQUIRE(isEqual == (iValue == jValue));
                REQUIRE(isLess == (iValue < jValue));
                REQUIRE(isLessOrEqual == (iValue <= jValue));
                REQUIRE(isGreater == (iValue > jValue));
                REQUIRE(isGreaterOrEqual == (iValue >= jValue));
                REQUIRE((iValue <=> jValue) == (i <=> j));
            }
        }
    }

    template <typename tText>
    static auto paddedString(const tText &text, std::size_t width) -> std::string {
        const auto characterCount = el::unittest::ConsoleLine::utf8Length(text);
        if (characterCount >= width) {
            return std::string{text};
        }
        std::string result;
        result.reserve(text.size() + (width - characterCount) + 1);
        result += text;
        result += std::string(width - characterCount, ' ');
        return result;
    }

    [[nodiscard]] static auto reverseCompareWithQM(const std::string_view &pattern, const std::string_view &str)
        -> bool {

        auto strIt = str.rbegin();
        auto patternIt = pattern.rbegin();
        for (; strIt != str.rend() && patternIt != pattern.rend(); ++strIt, ++patternIt) {
            if (*patternIt == '?') {
                continue;
            }
            if (*strIt != *patternIt) {
                return false;
            }
        }
        return patternIt == pattern.rend();
    }

    [[nodiscard]] static auto compareWithQM(
        const std::string_view &pattern, const std::string_view &str, const bool fullMatch) -> bool {

        auto strIt = str.begin();
        auto patternIt = pattern.begin();
        for (; strIt != str.end() && patternIt != pattern.end(); ++strIt, ++patternIt) {
            if (*patternIt == '?') {
                continue;
            }
            if (*strIt != *patternIt) {
                return false;
            }
        }
        if (fullMatch && strIt != str.end()) {
            return false;
        }
        if (patternIt != pattern.end()) {
            return false;
        }
        return true;
    }

    [[nodiscard]] static auto compareWithStar(const std::string_view &pattern, const std::string_view &str) -> bool {
        const auto pos = pattern.find('*');
        if (pos == std::string_view::npos) {
            return compareWithQM(pattern, str, true);
        }

        const auto prefix = pattern.substr(0, pos);
        if (!compareWithQM(prefix, str, false)) {
            return false;
        }

        const auto suffix = pattern.substr(pos + 1);
        if (suffix.empty()) {
            return true;
        }
        return reverseCompareWithQM(suffix, str);
    }

    [[nodiscard]] static auto compareWithStar(const std::string_view &pattern, const el::text::StringView &str)
        -> bool {
        return compareWithStar(pattern, string_helper::toStdString(str));
    }

    template <typename tExpected>
        requires std::ranges::range<tExpected>
    void requireLines(const el::text::StringViewList &actual, const tExpected &expected) {
        std::vector<std::string> standardLines;
        standardLines.reserve(actual.count().toSizeT());
        for (const auto &line : actual) {
            standardLines.emplace_back(string_helper::toStdString(line));
        }
        requireLines(standardLines, expected);
    }

    template <typename tActual, typename tExpected>
        requires std::ranges::range<tActual> && std::ranges::range<tExpected>
    void requireLines(const tActual &actual, const tExpected &expected) {

        runWithContext(
            SOURCE_LOCATION(),
            [&] {
                REQUIRE_EQUAL(actual.size(), expected.size());
                auto itActual = actual.begin();
                auto itExpected = expected.begin();
                for (; itActual != actual.end(); ++itActual, ++itExpected) {
                    REQUIRE(compareWithStar(*itExpected, *itActual));
                }
            },
            [&]() -> std::string {
                // display a side-by-side comparison.
                std::string message;
                const auto actualWidth =
                    std::max_element(actual.begin(), actual.end(), [](const auto &a, const auto &b) {
                        return a.size() < b.size();
                    })->size();
                const auto expectedWidth =
                    std::max_element(expected.begin(), expected.end(), [](const auto &a, const auto &b) {
                        return a.size() < b.size();
                    })->size();
                message += std::string{"| "};
                message += paddedString(std::format("Actual ({})", actual.size()), actualWidth);
                message += " |     | ";
                message += paddedString(std::format("Expected ({})", expected.size()), expectedWidth);
                message += " |\n";
                message += std::string{"|-"};
                message += std::string(actualWidth, '-');
                message += "-|-----|-";
                message += std::string(expectedWidth, '-');
                message += "-|\n";
                auto itActual = actual.begin();
                auto itExpected = expected.begin();
                while (itActual != actual.end() || itExpected != expected.end()) {
                    message += std::string{"| "};
                    if (itActual != actual.end()) {
                        message += paddedString(*itActual, actualWidth);
                    } else {
                        message += paddedString(std::string{}, actualWidth);
                    }

                    const auto areEqual =
                        itActual != actual.end() && itExpected != expected.end() && *itActual == *itExpected;
                    if (areEqual) {
                        message += " | === | ";
                    } else {
                        message += " |  X  | ";
                    }
                    if (itExpected != expected.end()) {
                        message += paddedString(*itExpected, expectedWidth);
                    } else {
                        message += paddedString(std::string{}, expectedWidth);
                    }
                    message += " |\n";

                    if (itActual != actual.end()) {
                        ++itActual;
                    }
                    if (itExpected != expected.end()) {
                        ++itExpected;
                    }
                }
                message += std::string{"|-"};
                message += std::string(actualWidth, '-');
                message += "-|-----|-";
                message += std::string(expectedWidth, '-');
                message += "-|\n";
                return message;
            });
    }
};

}
