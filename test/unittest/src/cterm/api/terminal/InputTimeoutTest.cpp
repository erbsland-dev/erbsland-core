// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cterm/impl/InputTimeout.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(InputTimeout)
class InputTimeoutTest final : public el::UnitTest {
public:
    void testFiniteDeadlineAndRounding() {
        const auto start = el::time::TimePoint{};
        const auto timeout = el::cterm::impl::InputTimeout{el::time::Milliseconds{125}, start};
        REQUIRE_EQUAL(*timeout.remaining(start), el::time::Milliseconds{125});
        REQUIRE_EQUAL(*timeout.remaining(start + el::time::Microseconds{124001}), el::time::Milliseconds{1});
        REQUIRE(timeout.expired(start + el::time::Milliseconds{125}));
        REQUIRE_EQUAL(*timeout.remaining(start + el::time::Milliseconds{200}), el::time::Milliseconds{});
    }
    void testUnboundedAndNegativeTimeouts() {
        const auto start = el::time::TimePoint{};
        REQUIRE_FALSE(el::cterm::impl::InputTimeout{}.remaining(start).has_value());
        const auto timeout = el::cterm::impl::InputTimeout{el::time::Milliseconds{-1}, start};
        REQUIRE(timeout.expired(start));
        REQUIRE_EQUAL(*timeout.remaining(start), el::time::Milliseconds{});
    }
    void testNativeConversionsDoNotOverflow() {
        using el::cterm::impl::InputTimeout;
        REQUIRE_EQUAL(InputTimeout::windowsMilliseconds(el::time::Milliseconds::maximum()), 0xfffffffeU);
        REQUIRE_EQUAL(InputTimeout::windowsMilliseconds(el::time::Milliseconds{-1}), 0U);
        REQUIRE_EQUAL(InputTimeout::windowsMilliseconds(el::time::Milliseconds{125}), 125U);
        const auto [seconds, micros] = InputTimeout::posixParts(el::time::Milliseconds::maximum());
        REQUIRE_EQUAL(seconds, el::time::Milliseconds::maximum().toRawValue() / 1000);
        REQUIRE(micros >= 0 && micros < 1'000'000);
    }
    void testHugeDeadlineSaturatesWithoutWrapping() {
        const auto start = el::time::TimePoint{} + el::time::Seconds{100};
        const auto timeout = el::cterm::impl::InputTimeout{el::time::Milliseconds::maximum(), start};
        REQUIRE_FALSE(timeout.expired(start));
        REQUIRE(timeout.remaining(start)->isPositive());
    }
};
