// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CaseSensitivity.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>

using namespace el::text;
using namespace el::text::literals;

TESTED_TARGETS(CaseSensitivity)
class CaseSensitivityTest final : public el::UnitTest {
public:
    void testConstructionAndValues() {
        static constexpr auto defaultValue = CaseSensitivity{};
        static constexpr auto insensitive = CaseSensitivity{CaseSensitivity::CaseInsensitive};

        static_assert(defaultValue == CaseSensitivity::CaseSensitive);
        static_assert(insensitive == CaseSensitivity::CaseInsensitive);
        static_assert(insensitive.toRawValue() == CaseSensitivity::CaseInsensitive);
        static_assert(static_cast<std::uint8_t>(CaseSensitivity::CaseSensitive) == 0U);
        static_assert(static_cast<std::uint8_t>(CaseSensitivity::CaseInsensitive) == 1U);
    }

    void testComparisonFunctions() {
        const auto sensitive = CaseSensitivity{CaseSensitivity::CaseSensitive};
        const auto insensitive = CaseSensitivity{CaseSensitivity::CaseInsensitive};

        REQUIRE(sensitive.comparisonFn() == nullptr);
        REQUIRE(sensitive.asciiComparisonFn() == nullptr);
        REQUIRE(String{"A"_el}.compare("a"_el, insensitive.comparisonFn()) == std::strong_ordering::equal);
        REQUIRE(String{"A"_el}.compare("a"_el, insensitive.asciiComparisonFn()) == std::strong_ordering::equal);
        REQUIRE(String{"Ä"_el}.compare("ä"_el, insensitive.comparisonFn()) == std::strong_ordering::equal);
        REQUIRE(String{"Ä"_el}.compare("ä"_el, insensitive.asciiComparisonFn()) != std::strong_ordering::equal);
    }

    void testToString() {
        REQUIRE_EQUAL(CaseSensitivity{CaseSensitivity::CaseSensitive}.toString(), "case-sensitive"_el);
        REQUIRE_EQUAL(CaseSensitivity{CaseSensitivity::CaseInsensitive}.toString(), "case-insensitive"_el);
    }
};
