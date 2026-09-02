// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/CoordinateSpan.hpp>
#include <erbsland/block/MarginPair.hpp>
#include <erbsland/block/Margins.hpp>
#include <erbsland/block/Position.hpp>
#include <erbsland/block/Rectangle.hpp>
#include <erbsland/block/Size.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/geometry/AxisMapper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::geometry::Axis;
using el::geometry::AxisMappable;
using el::geometry::AxisMapper;
using el::geometry::AxisMapping;
using el::geometry::Dimensionality;
using el::geometry::Orientation;
using el::geometry::SignedAxis;
using el::geometry::SignedAxisMapping;

struct ThreeDimensionalValue final {
    using AxisComponent = int;
    static constexpr auto cDimensionality = Dimensionality::Three;

    int x;
    int y;
    int z;

    auto operator==(const ThreeDimensionalValue &) const noexcept -> bool = default;

    [[nodiscard]] constexpr auto component(const Axis axis) const -> int {
        switch (axis) {
        case Axis::X:
            return x;
        case Axis::Y:
            return y;
        case Axis::Z:
            return z;
        }
        throw el::err::ParameterError{"Invalid axis.", "axis"};
    }

    [[nodiscard]] constexpr auto component(const SignedAxis axis) const -> int {
        const auto result = component(axis.axis());
        return axis.isReversed() ? -result : result;
    }
};

struct NotAxisMappable final {};

static_assert(AxisMappable<el::block::CoordinateSpan>);
static_assert(AxisMappable<el::block::MarginPair>);
static_assert(AxisMappable<el::block::Margins>);
static_assert(AxisMappable<el::block::Position>);
static_assert(AxisMappable<el::block::Rectangle>);
static_assert(AxisMappable<el::block::Size>);
static_assert(AxisMappable<ThreeDimensionalValue>);
static_assert(!AxisMappable<NotAxisMappable>);

static_assert(AxisMapper{Orientation::Horizontal}.map(el::block::Position{3, 7}) == el::block::Position{3, 7});
static_assert(AxisMapper{Orientation::Vertical}.map(el::block::Size{3, 7}) == el::block::Size{7, 3});

TESTED_TARGETS(AxisMapper AxisMapping SignedAxis SignedAxisMapping)
class AxisMapperTest final : public el::UnitTest {
public:
    void testMappingDescriptors() {
        const auto one = AxisMapping{Axis::X};
        const auto two = AxisMapping{Axis::Y, Axis::X};
        const auto three = AxisMapping{Axis::Z, Axis::X, Axis::Y};

        REQUIRE_EQUAL(one.dimensionality(), Dimensionality::One);
        REQUIRE(one.isIdentity());
        REQUIRE_EQUAL(two.dimensionality(), Dimensionality::Two);
        REQUIRE_EQUAL(two.source(Axis::X), Axis::Y);
        REQUIRE_EQUAL(two.source(Axis::Y), Axis::X);
        REQUIRE_FALSE(two.isIdentity());
        REQUIRE_EQUAL(three.dimensionality(), Dimensionality::Three);
        REQUIRE_EQUAL(three.source(Axis::Z), Axis::Y);

        const auto signedMapping = SignedAxisMapping{SignedAxis::NegativeY, SignedAxis::PositiveX};
        REQUIRE_EQUAL(signedMapping.source(Axis::X), SignedAxis::NegativeY);
        REQUIRE(signedMapping.source(Axis::X).isReversed());
        REQUIRE_EQUAL(signedMapping.source(Axis::X).axis(), Axis::Y);
    }

    void testOrientationMappings() {
        const auto horizontal = AxisMapper{Orientation::Horizontal};
        const auto vertical = AxisMapper{Orientation::Vertical};

        REQUIRE(horizontal.mapping().isIdentity());
        REQUIRE_FALSE(vertical.mapping().isIdentity());
        REQUIRE_EQUAL(horizontal.map(el::block::Position{3, 7}), (el::block::Position{3, 7}));
        REQUIRE_EQUAL(vertical.map(el::block::Position{3, 7}), (el::block::Position{7, 3}));
        REQUIRE_EQUAL(vertical.map(el::block::Size{12, 5}), (el::block::Size{5, 12}));
    }

    void testSignedBlockMappings() {
        const auto mapper = AxisMapper{SignedAxisMapping{SignedAxis::NegativeY, SignedAxis::PositiveX}};

        REQUIRE_EQUAL(mapper.map(el::block::Position{3, 7}), (el::block::Position{-7, 3}));
        REQUIRE_EQUAL(mapper.map(el::block::Size{3, 7}), (el::block::Size{7, 3}));
        REQUIRE_EQUAL(mapper.map(el::block::Rectangle{2, 5, 3, 4}), (el::block::Rectangle{-8, 2, 4, 3}));
        REQUIRE_EQUAL(mapper.map(el::block::Margins{1, 2, 3, 4}), (el::block::Margins{4, 1, 2, 3}));
    }

    void testRectangleMappingPreservesCells() {
        const auto mapper = AxisMapper{SignedAxisMapping{SignedAxis::NegativeY, SignedAxis::PositiveX}};
        const auto source = el::block::Rectangle{2, 5, 3, 4};
        const auto target = mapper.map(source);

        source.forEach(
            [&](const el::block::Position position) -> void { REQUIRE(target.contains(mapper.map(position))); });
    }

    void testOneAndThreeDimensionalMappings() {
        const auto pairMapper = AxisMapper{SignedAxisMapping{SignedAxis::NegativeX}};
        REQUIRE_EQUAL(pairMapper.map(el::block::MarginPair{2, 7}), (el::block::MarginPair{7, 2}));
        REQUIRE_EQUAL(pairMapper.map(el::block::CoordinateSpan{4, 3}), (el::block::CoordinateSpan{-6, 3}));

        const auto valueMapper =
            AxisMapper{SignedAxisMapping{SignedAxis::NegativeZ, SignedAxis::PositiveX, SignedAxis::PositiveY}};
        REQUIRE_EQUAL(valueMapper.map(ThreeDimensionalValue{2, 3, 5}), (ThreeDimensionalValue{-5, 2, 3}));
    }

    void testInvalidMappings() {
        REQUIRE_THROWS_AS(el::err::ParameterError, (AxisMapping{Axis::X, Axis::X}));
        REQUIRE_THROWS_AS(el::err::ParameterError, (AxisMapping{Axis::X, Axis::Z}));
        REQUIRE_THROWS_AS(el::err::ParameterError, (SignedAxisMapping{SignedAxis::NegativeX, SignedAxis::PositiveX}));

        const auto invalidSignedAxis = SignedAxis{static_cast<Axis>(99)};
        REQUIRE_FALSE(invalidSignedAxis.isValid());
        REQUIRE_FALSE(invalidSignedAxis.isReversed());
        REQUIRE_THROWS_AS(el::err::ParameterError, (SignedAxisMapping{invalidSignedAxis}));
        REQUIRE_THROWS_AS(el::err::ParameterError, el::block::Position{1, 2}.component(invalidSignedAxis));

        const auto one = AxisMapping{Axis::X};
        REQUIRE_THROWS_AS(el::err::ParameterError, one.source(Axis::Y));
        REQUIRE_THROWS_AS(el::err::ParameterError, AxisMapper{AxisMapping{Axis::X}}.map(el::block::Position{1, 2}));
    }
};
