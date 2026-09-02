// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/MarginPair.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/geometry/Axis.hpp>
#include <erbsland/geometry/SignedAxis.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(MarginPair)
class MarginPairTest final : public el::UnitTest {
public:
    void testConstructionAndAccessors() {
        const auto uniform = el::block::MarginPair{3};
        REQUIRE_EQUAL(uniform.leading(), 3);
        REQUIRE_EQUAL(uniform.trailing(), 3);

        auto pair = el::block::MarginPair{2, 7};
        pair.setLeading(-4);
        pair.setTrailing(5);
        REQUIRE_EQUAL(pair.leading(), -4);
        REQUIRE_EQUAL(pair.trailing(), 5);
    }

    void testCalculationsAndReversal() {
        const auto pair = el::block::MarginPair{-3, 5};
        REQUIRE_EQUAL(pair.extent(), 5);
        REQUIRE_EQUAL(pair.delta(), 2);
        REQUIRE_EQUAL(pair.spacing(), 5);
        REQUIRE_EQUAL(pair.reversed(), (el::block::MarginPair{5, -3}));
        REQUIRE_EQUAL(-pair, (el::block::MarginPair{3, -5}));
        REQUIRE_EQUAL(pair.component(el::geometry::SignedAxis::NegativeX), pair.reversed());
    }

    void testConstraints() {
        auto pair = el::block::MarginPair{-3, 5};
        pair.expandTo(el::block::MarginPair{1, 2});
        REQUIRE_EQUAL(pair, (el::block::MarginPair{1, 5}));
        pair.limitTo(el::block::MarginPair{0, 4});
        REQUIRE_EQUAL(pair, (el::block::MarginPair{0, 4}));
        REQUIRE_EQUAL((el::block::MarginPair{-2, 3}.expandedPositive()), (el::block::MarginPair{0, 3}));
    }

    void testInvalidAxis() {
        const auto pair = el::block::MarginPair{2, 3};
        REQUIRE_THROWS_AS(el::err::ParameterError, pair.component(el::geometry::Axis::Z));
    }
};
