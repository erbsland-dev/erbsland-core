// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/impl/algorithm/NistPrimeNumber.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

TESTED_TARGETS(NistPrimeNumber)
class NistPrimeNumberTest final : public el::UnitTest {
public:
    void testCopyMutationAndErasure() {
        using el::cryptology::impl::NistPrimeNumber;
        auto parameters = std::array<uint32_t, 12U>{};
        parameters[0] = 42U;
        parameters[11] = 123U;
        const auto original = NistPrimeNumber{parameters};
        auto copy = original;
        copy.words()[0] = 7U;
        REQUIRE_EQUAL(original.words()[0], 42U);
        REQUIRE_EQUAL(copy.words()[0], 7U);
        copy = original;
        REQUIRE_EQUAL(copy.words()[11], 123U);
        const auto &same = copy;
        copy = same;
        REQUIRE_EQUAL(copy.words()[0], 42U);
        copy.secureErase();
        for (const auto word : copy.words()) {
            REQUIRE_EQUAL(word, 0U);
        }
        REQUIRE_EQUAL(original.words()[11], 123U);
    }

    void testZeroConstruction() {
        const auto value = el::cryptology::impl::NistPrimeNumber{};
        for (const auto word : value.words()) {
            REQUIRE_EQUAL(word, 0U);
        }
    }
};
