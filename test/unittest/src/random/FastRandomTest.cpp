// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/random/FastRandom.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>

using el::random::FastRandom;

TESTED_TARGETS(FastRandom)
class FastRandomTest final : public el::UnitTest {
public:
    void testSeededReproducibility() {
        auto first = FastRandom{12345U};
        auto second = FastRandom{12345U};

        for (auto i = 0; i < 20; ++i) {
            REQUIRE_EQUAL(first.getUInt64(0U, 1000000U), second.getUInt64(0U, 1000000U));
        }
    }

    void testRangeSmoke() {
        auto random = FastRandom{};

        for (auto i = 0; i < 100; ++i) {
            const auto signedValue = random.getInt32(-3, 3);
            REQUIRE(signedValue >= -3);
            REQUIRE(signedValue <= 3);

            const auto unsignedValue = random.getUInt64(5U, 10U);
            REQUIRE(unsignedValue >= 5U);
            REQUIRE(unsignedValue <= 10U);
        }
    }

    void testFillBytes() {
        auto random = FastRandom{7U};
        auto bytes = std::array<std::byte, 16>{};

        random.fillBytes(bytes);
        REQUIRE_NOTHROW(random.fillBytes({}));
    }
};
