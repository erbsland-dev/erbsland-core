// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Coordinate.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <limits>

using el::math::SatInt32;

TESTED_TARGETS(Coordinate)
class CoordinateTest final : public el::UnitTest {
public:
    void testTypeContract() {
        using el::block::Coordinate;

        static_assert(std::same_as<Coordinate, SatInt32>);
    }

    void testSaturatingArithmetic() {
        using el::block::Coordinate;

        const auto maximum = Coordinate::maximum();
        const auto minimum = Coordinate::minimum();

        const auto saturatedMaximum = maximum + Coordinate{1};
        const auto saturatedMinimum = minimum - Coordinate{1};
        const auto absolute = Coordinate{-12}.toAbsolute();
        const auto clamped = Coordinate{8}.clamped(Coordinate{2}, Coordinate{5});
        REQUIRE_EQUAL(saturatedMaximum.toRawValue(), maximum.toRawValue());
        REQUIRE_EQUAL(saturatedMinimum.toRawValue(), minimum.toRawValue());
        REQUIRE_EQUAL(absolute.toRawValue(), 12);
        REQUIRE_EQUAL(clamped.toRawValue(), 5);
    }
};
