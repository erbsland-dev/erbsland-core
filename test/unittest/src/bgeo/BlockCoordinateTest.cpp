// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/BlockCoordinate.hpp>
#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <limits>

using el::math::SatInt32;

TESTED_TARGETS(BlockCoordinate)
class BlockCoordinateTest final : public el::UnitTest {
public:
    void testTypeContract() {
        using el::bgeo::BlockCoordinate;

        static_assert(std::same_as<BlockCoordinate, SatInt32>);
    }

    void testSaturatingArithmetic() {
        using el::bgeo::BlockCoordinate;

        const auto maximum = BlockCoordinate::maximum();
        const auto minimum = BlockCoordinate::minimum();

        const auto saturatedMaximum = maximum + BlockCoordinate{1};
        const auto saturatedMinimum = minimum - BlockCoordinate{1};
        const auto absolute = BlockCoordinate{-12}.toAbsolute();
        const auto clamped = BlockCoordinate{8}.clamped(BlockCoordinate{2}, BlockCoordinate{5});
        REQUIRE_EQUAL(saturatedMaximum.toRawValue(), maximum.toRawValue());
        REQUIRE_EQUAL(saturatedMinimum.toRawValue(), minimum.toRawValue());
        REQUIRE_EQUAL(absolute.toRawValue(), 12);
        REQUIRE_EQUAL(clamped.toRawValue(), 5);
    }
};
