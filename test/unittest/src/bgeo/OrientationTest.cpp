// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/Orientation.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <type_traits>

using el::bgeo::Orientation;

static_assert(std::is_same_v<decltype(Orientation{}.value()), Orientation::Value>);
static_assert(Orientation{}.value() == Orientation::Horizontal);
static_assert(Orientation{Orientation::Horizontal}.crossed() == Orientation::Vertical);
static_assert(Orientation{Orientation::Vertical}.crossed() == Orientation::Horizontal);

TESTED_TARGETS(Orientation)
class OrientationTest final : public el::UnitTest {
public:
    void testDefaultConstructor() { REQUIRE_EQUAL(Orientation{}.value(), Orientation::Horizontal); }

    void testConstructorFromValue() {
        const auto orientation = Orientation{Orientation::Vertical};

        REQUIRE_EQUAL(orientation.value(), Orientation::Vertical);
    }

    void testCrossed() {

        REQUIRE_EQUAL(Orientation{Orientation::Horizontal}.crossed(), Orientation::Vertical);
        REQUIRE_EQUAL(Orientation{Orientation::Vertical}.crossed(), Orientation::Horizontal);
    }
};
