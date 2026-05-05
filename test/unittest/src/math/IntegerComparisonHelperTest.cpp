// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/impl/IntegerComparisonHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <utility>

namespace erbsland::math::impl {

class MixedIntegerComparisonProbe final {
public:
    explicit constexpr MixedIntegerComparisonProbe(std::int8_t value) noexcept : _value{value} {}

    ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE(_value);
    ERBSLAND_CORE_CONSTEXPR_MIXED_INTEGER_COMPARE_FRIEND(
        _value, const MixedIntegerComparisonProbe &probe, probe._value);

private:
    std::int8_t _value;
};

}

template <typename T>
concept ProbeCanCompareMember = requires(const erbsland::math::impl::MixedIntegerComparisonProbe &probe, T value) {
    { probe == value } -> std::same_as<bool>;
};

template <typename T>
concept ProbeCanCompareFriend = requires(T value, const erbsland::math::impl::MixedIntegerComparisonProbe &probe) {
    { value == probe } -> std::same_as<bool>;
};

TESTED_TARGETS(IntegerComparisonHelper)
class IntegerComparisonHelperTest final : public el::UnitTest {
    using Probe = erbsland::math::impl::MixedIntegerComparisonProbe;

public:
    void testMemberComparisonContracts() {
        static_assert(std::same_as<
            decltype(std::declval<const Probe &>() <=> std::declval<std::uint64_t>()),
            std::strong_ordering>);
        static_assert(noexcept(std::declval<const Probe &>() <=> std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() == std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() != std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() < std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() <= std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() > std::declval<std::uint64_t>()));
        static_assert(noexcept(std::declval<const Probe &>() >= std::declval<std::uint64_t>()));
        static_assert(ProbeCanCompareMember<std::int8_t>);
        static_assert(ProbeCanCompareMember<std::uint64_t>);
        static_assert(!ProbeCanCompareMember<bool>);

        static_assert((Probe{-1} <=> std::uint8_t{0}) == std::strong_ordering::less);
        static_assert((Probe{0} <=> std::uint8_t{0}) == std::strong_ordering::equal);
        static_assert((Probe{1} <=> std::uint8_t{0}) == std::strong_ordering::greater);
        static_assert(Probe{-1} != std::uint64_t{std::numeric_limits<std::uint64_t>::max()});
        static_assert(Probe{-1} < std::uint8_t{0});
        static_assert(Probe{0} <= std::uint8_t{0});
        static_assert(Probe{1} > std::uint8_t{0});
        static_assert(Probe{1} >= std::uint8_t{0});

        const auto probe = Probe{-1};
        REQUIRE((probe <=> std::uint64_t{std::numeric_limits<std::uint64_t>::max()}) == std::strong_ordering::less);
        REQUIRE(probe == std::int8_t{-1});
        REQUIRE_FALSE(probe == std::uint8_t{0});
        REQUIRE(probe != std::uint8_t{0});
        REQUIRE(probe < std::uint8_t{0});
        REQUIRE(probe <= std::int8_t{-1});
        REQUIRE_FALSE(probe > std::uint8_t{0});
        REQUIRE_FALSE(probe >= std::uint8_t{0});
    }

    void testFriendComparisonContracts() {
        static_assert(std::same_as<
            decltype(std::declval<std::uint64_t>() <=> std::declval<const Probe &>()),
            std::strong_ordering>);
        static_assert(noexcept(std::declval<std::uint64_t>() <=> std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() == std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() != std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() < std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() <= std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() > std::declval<const Probe &>()));
        static_assert(noexcept(std::declval<std::uint64_t>() >= std::declval<const Probe &>()));
        static_assert(ProbeCanCompareFriend<std::int8_t>);
        static_assert(ProbeCanCompareFriend<std::uint64_t>);
        static_assert(!ProbeCanCompareFriend<bool>);

        static_assert((std::uint8_t{0} <=> Probe{-1}) == std::strong_ordering::greater);
        static_assert((std::uint8_t{0} <=> Probe{0}) == std::strong_ordering::equal);
        static_assert((std::uint8_t{0} <=> Probe{1}) == std::strong_ordering::less);
        static_assert(std::uint8_t{0} != Probe{-1});
        static_assert(std::uint8_t{0} < Probe{1});
        static_assert(std::uint8_t{0} <= Probe{0});
        static_assert(std::uint8_t{0} > Probe{-1});
        static_assert(std::uint8_t{0} >= Probe{0});

        const auto probe = Probe{-1};
        REQUIRE((std::uint64_t{std::numeric_limits<std::uint64_t>::max()} <=> probe) == std::strong_ordering::greater);
        REQUIRE(std::int8_t{-1} == probe);
        REQUIRE_FALSE(std::uint8_t{0} == probe);
        REQUIRE(std::uint8_t{0} != probe);
        REQUIRE_FALSE(std::uint8_t{0} < probe);
        REQUIRE_FALSE(std::uint8_t{0} <= probe);
        REQUIRE(std::uint8_t{0} > probe);
        REQUIRE(std::uint8_t{0} >= probe);
    }
};
