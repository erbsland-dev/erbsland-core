// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/StdFormat.hpp>
#include <erbsland/geometry/Orientation.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <type_traits>

using el::geometry::Orientation;

static_assert(std::is_same_v<decltype(Orientation{}.value()), Orientation::Value>);
static_assert(Orientation{}.value() == Orientation::Horizontal);
static_assert(Orientation{Orientation::Horizontal}.crossed() == Orientation::Vertical);
static_assert(Orientation{Orientation::Vertical}.crossed() == Orientation::Horizontal);

TESTED_TARGETS(Orientation)
class OrientationTest final : public el::UnitTest {
public:
    void testDefaultConstructor() {
        const auto orientation = Orientation{};
        REQUIRE_EQUAL(orientation.value(), Orientation::Horizontal);
    }

    void testConstructorFromValue() {
        const auto orientation = Orientation{Orientation::Vertical};

        REQUIRE_EQUAL(orientation.value(), Orientation::Vertical);
    }

    void testCrossed() {

        const auto horizontal = Orientation{Orientation::Horizontal};
        const auto vertical = Orientation{Orientation::Vertical};
        REQUIRE_EQUAL(horizontal.crossed(), Orientation::Vertical);
        REQUIRE_EQUAL(vertical.crossed(), Orientation::Horizontal);
    }
};
