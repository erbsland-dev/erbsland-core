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

        REQUIRE_EQUAL((maximum + BlockCoordinate{1}).toRawValue(), maximum.toRawValue());
        REQUIRE_EQUAL((minimum - BlockCoordinate{1}).toRawValue(), minimum.toRawValue());
        REQUIRE_EQUAL(BlockCoordinate{-12}.toAbsolute().toRawValue(), 12);
        REQUIRE_EQUAL(BlockCoordinate{8}.clamped(BlockCoordinate{2}, BlockCoordinate{5}).toRawValue(), 5);
    }
};
