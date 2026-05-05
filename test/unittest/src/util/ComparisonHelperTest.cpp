// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/impl/ComparisonHelper.hpp>

#include <compare>
#include <concepts>
#include <utility>

class RuntimeComparisonProbe final {
public:
    explicit RuntimeComparisonProbe(int value) noexcept : _value{value} {}

    ERBSLAND_CORE_COMPARE_MEMBER(_value, const RuntimeComparisonProbe &other, other._value);

private:
    int _value;
};

class ConstexprComparisonProbe final {
public:
    explicit constexpr ConstexprComparisonProbe(int value) noexcept : _value{value} {}

    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, int other, other);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(int value, const ConstexprComparisonProbe &probe, value, probe._value);

private:
    int _value;
};

class ConstexprSpaceshipProbe final {
public:
    explicit constexpr ConstexprSpaceshipProbe(int value) noexcept : _value{value} {}

    constexpr auto operator<=>(const ConstexprSpaceshipProbe &other) const noexcept -> std::strong_ordering {
        return _value <=> other._value;
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const ConstexprSpaceshipProbe &other, other);

private:
    int _value;
};

TESTED_TARGETS(ComparisonHelper)
class ComparisonHelperTest final : public el::UnitTest {
public:
    void testRuntimeMemberComparison() {
        static_assert(std::same_as<
            decltype(std::declval<const RuntimeComparisonProbe &>() <=> std::declval<const RuntimeComparisonProbe &>()),
            std::strong_ordering>);
        static_assert(noexcept(
            std::declval<const RuntimeComparisonProbe &>() <=> std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() == std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() != std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() < std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() <= std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() > std::declval<const RuntimeComparisonProbe &>()));
        static_assert(
            noexcept(std::declval<const RuntimeComparisonProbe &>() >= std::declval<const RuntimeComparisonProbe &>()));

        const auto one = RuntimeComparisonProbe{1};
        const auto anotherOne = RuntimeComparisonProbe{1};
        const auto two = RuntimeComparisonProbe{2};

        REQUIRE((one <=> anotherOne) == std::strong_ordering::equal);
        REQUIRE((one <=> two) == std::strong_ordering::less);
        REQUIRE((two <=> one) == std::strong_ordering::greater);

        REQUIRE(one == anotherOne);
        REQUIRE_FALSE(one != anotherOne);
        REQUIRE(one != two);
        REQUIRE(one < two);
        REQUIRE(one <= anotherOne);
        REQUIRE(one <= two);
        REQUIRE(two > one);
        REQUIRE(two >= anotherOne);
        REQUIRE(two >= one);
        REQUIRE_FALSE(two < one);
        REQUIRE_FALSE(one > two);
    }

    void testConstexprMemberComparison() {
        static_assert(ConstexprComparisonProbe{7} == 7);
        static_assert(ConstexprComparisonProbe{7} != 8);
        static_assert(ConstexprComparisonProbe{7} < 8);
        static_assert(ConstexprComparisonProbe{7} <= 7);
        static_assert(ConstexprComparisonProbe{7} <= 8);
        static_assert(ConstexprComparisonProbe{7} > 6);
        static_assert(ConstexprComparisonProbe{7} >= 7);
        static_assert(ConstexprComparisonProbe{7} >= 6);
        static_assert((ConstexprComparisonProbe{7} <=> 7) == std::strong_ordering::equal);
        static_assert((ConstexprComparisonProbe{7} <=> 8) == std::strong_ordering::less);
        static_assert((ConstexprComparisonProbe{7} <=> 6) == std::strong_ordering::greater);
        static_assert(noexcept(ConstexprComparisonProbe{7} == 7));
        static_assert(noexcept(ConstexprComparisonProbe{7} < 8));

        const auto probe = ConstexprComparisonProbe{7};
        REQUIRE((probe <=> 7) == std::strong_ordering::equal);
        REQUIRE(probe == 7);
        REQUIRE_FALSE(probe != 7);
        REQUIRE(probe != 8);
        REQUIRE(probe < 8);
        REQUIRE(probe <= 7);
        REQUIRE(probe > 6);
        REQUIRE(probe >= 7);
    }

    void testConstexprFriendComparison() {
        static_assert(7 == ConstexprComparisonProbe{7});
        static_assert(6 != ConstexprComparisonProbe{7});
        static_assert(6 < ConstexprComparisonProbe{7});
        static_assert(7 <= ConstexprComparisonProbe{7});
        static_assert(8 > ConstexprComparisonProbe{7});
        static_assert(7 >= ConstexprComparisonProbe{7});
        static_assert((7 <=> ConstexprComparisonProbe{7}) == std::strong_ordering::equal);
        static_assert((6 <=> ConstexprComparisonProbe{7}) == std::strong_ordering::less);
        static_assert((8 <=> ConstexprComparisonProbe{7}) == std::strong_ordering::greater);
        static_assert(noexcept(7 == ConstexprComparisonProbe{7}));
        static_assert(noexcept(6 < ConstexprComparisonProbe{7}));

        const auto probe = ConstexprComparisonProbe{7};
        REQUIRE((7 <=> probe) == std::strong_ordering::equal);
        REQUIRE(7 == probe);
        REQUIRE_FALSE(6 == probe);
        REQUIRE(6 != probe);
        REQUIRE(6 < probe);
        REQUIRE(7 <= probe);
        REQUIRE(8 > probe);
        REQUIRE(7 >= probe);
    }

    void testConstexprComparisonFromSpaceship() {
        static_assert(ConstexprSpaceshipProbe{7} == ConstexprSpaceshipProbe{7});
        static_assert(ConstexprSpaceshipProbe{7} != ConstexprSpaceshipProbe{8});
        static_assert(ConstexprSpaceshipProbe{7} < ConstexprSpaceshipProbe{8});
        static_assert(ConstexprSpaceshipProbe{7} <= ConstexprSpaceshipProbe{7});
        static_assert(ConstexprSpaceshipProbe{7} <= ConstexprSpaceshipProbe{8});
        static_assert(ConstexprSpaceshipProbe{7} > ConstexprSpaceshipProbe{6});
        static_assert(ConstexprSpaceshipProbe{7} >= ConstexprSpaceshipProbe{7});
        static_assert(ConstexprSpaceshipProbe{7} >= ConstexprSpaceshipProbe{6});

        const auto seven = ConstexprSpaceshipProbe{7};
        const auto anotherSeven = ConstexprSpaceshipProbe{7};
        const auto eight = ConstexprSpaceshipProbe{8};
        REQUIRE(seven == anotherSeven);
        REQUIRE_FALSE(seven != anotherSeven);
        REQUIRE(seven < eight);
        REQUIRE(seven <= anotherSeven);
        REQUIRE(eight > seven);
        REQUIRE(eight >= anotherSeven);
    }
};
