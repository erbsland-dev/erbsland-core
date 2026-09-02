// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/CoordinateSpan.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/geometry/Axis.hpp>
#include <erbsland/geometry/SignedAxis.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(CoordinateSpan)
class CoordinateSpanTest final : public el::UnitTest {
public:
    void testConstructionAndAccessors() {
        auto span = el::block::CoordinateSpan{3, 5};
        REQUIRE_EQUAL(span.origin(), 3);
        REQUIRE_EQUAL(span.extent(), 5);
        REQUIRE_EQUAL(span.end(), 8);

        span.setOrigin(-4);
        span.setExtent(-2);
        REQUIRE_EQUAL(span.origin(), -4);
        REQUIRE_EQUAL(span.extent(), 0);
        REQUIRE(span.isEmpty());
    }

    void testHalfOpenContainment() {
        const auto span = el::block::CoordinateSpan{-2, 4};
        REQUIRE_FALSE(span.contains(el::block::Coordinate{-3}));
        REQUIRE(span.contains(el::block::Coordinate{-2}));
        REQUIRE(span.contains(el::block::Coordinate{1}));
        REQUIRE_FALSE(span.contains(el::block::Coordinate{2}));
    }

    void testReversal() {
        const auto reversed = el::block::CoordinateSpan{4, 3}.reversed();
        REQUIRE_EQUAL(reversed, (el::block::CoordinateSpan{-6, 3}));
        REQUIRE(reversed.contains(el::block::Coordinate{-4}));
        REQUIRE(reversed.contains(el::block::Coordinate{-5}));
        REQUIRE(reversed.contains(el::block::Coordinate{-6}));

        REQUIRE_EQUAL((el::block::CoordinateSpan{4, 0}.reversed()), (el::block::CoordinateSpan{-4, 0}));
        REQUIRE_EQUAL((el::block::CoordinateSpan{4, 3}.component(el::geometry::SignedAxis::NegativeX)), reversed);
    }

    void testInvalidAxis() {
        const auto span = el::block::CoordinateSpan{2, 3};
        REQUIRE_THROWS_AS(el::err::ParameterError, span.component(el::geometry::Axis::Y));
    }
};
