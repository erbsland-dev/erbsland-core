// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Integer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>
#include <type_traits>

using namespace el::conf;

TESTED_TARGETS(Integer)
class IntegerTest final : public el::UnitTest {
public:
    void testAlias() {
        // Integer should be a signed 64-bit type
        REQUIRE(std::is_same_v<Integer, std::int64_t>);
        REQUIRE(std::numeric_limits<Integer>::is_signed);
        REQUIRE(sizeof(Integer) == sizeof(std::int64_t));
    }
};
