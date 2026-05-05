// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/BoundedInteger.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

using el::math::BoundedInteger;
using el::math::IntegerRange;

TESTED_TARGETS(IntegerRange ClampedInteger)
class ClampedIntegerTest final : public el::UnitTest {
public:
    void testIntegerRange() {
        using SignedRange = IntegerRange<std::int8_t>;
        constexpr auto signedRange = SignedRange{10, -10};
        static_assert(signedRange.minimum() == -10);
        static_assert(signedRange.maximum() == 10);
        static_assert(signedRange.contains(-10));
        static_assert(signedRange.contains(10));
        static_assert(!signedRange.contains(-11));
        static_assert(!signedRange.contains(11U));
        static_assert(signedRange.clamped(-1000) == -10);
        static_assert(signedRange.clamped(1000U) == 10);
        static_assert(signedRange.contains(IntegerRange<std::int16_t>{-5, 5}));
        static_assert(!signedRange.contains(IntegerRange<std::int16_t>{-50, 5}));

        constexpr auto unsignedRange = IntegerRange<std::uint8_t>{10U, 20U};
        static_assert(unsignedRange.contains(10U));
        static_assert(unsignedRange.contains(20));
        static_assert(!unsignedRange.contains(-1));
        static_assert(unsignedRange.clamped(-1) == 10U);
        static_assert(unsignedRange.clamped(1000U) == 20U);
    }

    void testSignedClampedIntegers() {
        WITH_CONTEXT(requireSignedType<std::int8_t>());
        WITH_CONTEXT(requireSignedType<std::int16_t>());
        WITH_CONTEXT(requireSignedType<std::int32_t>());
        WITH_CONTEXT(requireSignedType<std::int64_t>());
    }

    void testUnsignedClampedIntegers() {
        WITH_CONTEXT(requireUnsignedType<std::uint8_t>());
        WITH_CONTEXT(requireUnsignedType<std::uint16_t>());
        WITH_CONTEXT(requireUnsignedType<std::uint32_t>());
        WITH_CONTEXT(requireUnsignedType<std::uint64_t>());
    }

private:
    template <typename Native>
    using SignedClamp = BoundedInteger<
        Native,
        static_cast<Native>(std::numeric_limits<Native>::min() + Native{10}),
        static_cast<Native>(std::numeric_limits<Native>::max() - Native{10}),
        Native{0}>;

    template <typename Native>
    using UnsignedClamp = BoundedInteger<
        Native,
        static_cast<Native>(10U),
        static_cast<Native>(std::numeric_limits<Native>::max() - Native{10}),
        static_cast<Native>(10U)>;

    template <typename Native>
    void requireSignedType() {
        using Clamp = SignedClamp<Native>;
        constexpr auto minimum = Clamp::minimumRawValue();
        constexpr auto maximum = Clamp::maximumRawValue();

        static_assert(std::same_as<typename Clamp::NativeValue, Native>);
        static_assert(Clamp::contains(minimum));
        static_assert(Clamp::contains(maximum));
        static_assert(!Clamp::contains(std::numeric_limits<Native>::min()));
        static_assert(!Clamp::contains(std::numeric_limits<Native>::max()));
        static_assert(Clamp::range().minimum() == minimum);
        static_assert(Clamp::range().maximum() == maximum);
        static_assert(Clamp{std::numeric_limits<Native>::min()}.isMinimum());
        static_assert(Clamp{std::numeric_limits<Native>::max()}.isMaximum());

        requireCommonApi<Clamp, Native>(minimum, maximum, Native{0});
        requireSignedSaturation<Clamp, Native>(minimum, maximum);
    }

    template <typename Native>
    void requireUnsignedType() {
        using Clamp = UnsignedClamp<Native>;
        constexpr auto minimum = Clamp::minimumRawValue();
        constexpr auto maximum = Clamp::maximumRawValue();

        static_assert(std::same_as<typename Clamp::NativeValue, Native>);
        static_assert(Clamp::contains(minimum));
        static_assert(Clamp::contains(maximum));
        static_assert(!Clamp::contains(Native{0}));
        static_assert(!Clamp::contains(-1));
        static_assert(Clamp::range().minimum() == minimum);
        static_assert(Clamp::range().maximum() == maximum);
        static_assert(Clamp{-1}.isMinimum());
        static_assert(Clamp{std::numeric_limits<std::uint64_t>::max()}.isMaximum());

        requireCommonApi<Clamp, Native>(minimum, maximum, minimum);
        requireUnsignedSaturation<Clamp, Native>(minimum, maximum);
    }

    template <typename Clamp, typename Native>
    void requireCommonApi(Native minimum, Native maximum, Native defaultValue) {
        REQUIRE_EQUAL(raw(Clamp{}.toRawValue()), raw(defaultValue));
        REQUIRE_EQUAL(raw(Clamp::minimum().toRawValue()), raw(minimum));
        REQUIRE_EQUAL(raw(Clamp::maximum().toRawValue()), raw(maximum));
        REQUIRE(Clamp{minimum}.isMinimum());
        REQUIRE(Clamp{maximum}.isMaximum());
        REQUIRE(Clamp{minimum} < Clamp{maximum});
        REQUIRE(Clamp{minimum}.compare(minimum) == std::strong_ordering::equal);
        REQUIRE(Clamp{minimum} == minimum);
        REQUIRE(Clamp{maximum} >= maximum);
        REQUIRE_EQUAL(raw(Clamp{minimum}.toValue().toRawValue()), raw(minimum));

        auto value = Clamp{maximum};
        ++value;
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(maximum));
        value = Clamp{minimum};
        --value;
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(minimum));

        value = Clamp{static_cast<Native>(minimum + Native{1})};
        const auto previous = value--;
        REQUIRE_EQUAL(raw(previous.toRawValue()), raw(static_cast<Native>(minimum + Native{1})));
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(minimum));
        const auto next = value++;
        REQUIRE_EQUAL(raw(next.toRawValue()), raw(minimum));
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(static_cast<Native>(minimum + Native{1})));
    }

    template <typename Clamp, typename Native>
    void requireSignedSaturation(Native minimum, Native maximum) {
        const auto nearMaximum = static_cast<Native>(maximum - Native{1});
        const auto nearMinimum = static_cast<Native>(minimum + Native{1});

        REQUIRE_EQUAL(raw((Clamp{nearMaximum} + Native{5}).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(raw((Clamp{nearMinimum} - Native{5}).toRawValue()), raw(minimum));
        REQUIRE_EQUAL(raw((Clamp{maximum} + Clamp{maximum}).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(raw((Clamp{minimum} - Clamp{maximum}).toRawValue()), raw(minimum));
        REQUIRE_EQUAL(raw(Clamp{0}.added(std::numeric_limits<std::uint64_t>::max()).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(raw(Clamp{0}.subtracted(std::numeric_limits<std::uint64_t>::max()).toRawValue()), raw(minimum));

        auto value = Clamp{nearMaximum};
        value += Native{5};
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(maximum));
        value = Clamp{nearMinimum};
        value -= Native{5};
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(minimum));
    }

    template <typename Clamp, typename Native>
    void requireUnsignedSaturation(Native minimum, Native maximum) {
        const auto nearMaximum = static_cast<Native>(maximum - Native{1});
        const auto nearMinimum = static_cast<Native>(minimum + Native{1});

        REQUIRE_EQUAL(raw((Clamp{nearMaximum} + Native{5}).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(raw((Clamp{nearMinimum} - Native{5}).toRawValue()), raw(minimum));
        REQUIRE_EQUAL(raw((Clamp{maximum} + Clamp{maximum}).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(raw((Clamp{minimum} - Clamp{maximum}).toRawValue()), raw(minimum));
        REQUIRE_EQUAL(
            raw(Clamp{nearMaximum}.added(std::numeric_limits<std::uint64_t>::max()).toRawValue()), raw(maximum));
        REQUIRE_EQUAL(
            raw(Clamp{nearMinimum}.added(std::numeric_limits<std::int64_t>::min()).toRawValue()), raw(minimum));

        auto value = Clamp{nearMaximum};
        value += Native{5};
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(maximum));
        value = Clamp{nearMinimum};
        value -= Native{5};
        REQUIRE_EQUAL(raw(value.toRawValue()), raw(minimum));
    }

    template <typename T>
    [[nodiscard]] auto raw(T value) -> std::uint64_t {
        if constexpr (std::signed_integral<T>) {
            return static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
        } else {
            return static_cast<std::uint64_t>(value);
        }
    }
};
