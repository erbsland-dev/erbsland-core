// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/random/RandomError.hpp>
#include <erbsland/random/SecureRandom.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>

using el::random::RandomError;
using el::random::SecureRandom;

TESTED_TARGETS(SecureRandom RandomError EntropySource)
class SecureRandomTest final : public el::UnitTest {
public:
    void testSecureRandomSmoke() {
        auto random = SecureRandom{};
        auto bytes = std::array<std::byte, 32>{};

        REQUIRE_NOTHROW(random.fillBytes(bytes));
        REQUIRE_NOTHROW(random.fillBytes({}));
        for (auto i = 0; i < 20; ++i) {
            const auto value = random.getUInt32(5U, 9U);
            REQUIRE_GREATER_EQUAL(value, 5U);
            REQUIRE_LESS_EQUAL(value, 9U);
        }
        const auto value = random.getDouble(1.0, 2.0);
        REQUIRE_GREATER_EQUAL(value, 1.0);
    }

    void testEntropySourceFailure() { REQUIRE_THROWS_AS(RandomError, throw RandomError{"test"}); }
};
