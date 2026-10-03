// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/PiDigitGenerator.hpp"

#include <erbsland/CpCount.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(PiDigitGenerator)
class PiDigitGeneratorTest final : public el::UnitTest {
public:
    void testExactPrefixThroughIncrementalSteps() {
        auto generator = demo::PiDigitGenerator{};
        auto result = el::StringEditor{};
        auto pendingSteps = 0;
        while (result.characterLength() < el::CpCount{101}) {
            const auto digit = generator.advance();
            if (digit) {
                REQUIRE(*digit >= U'0' && *digit <= U'9');
                result.append(*digit);
            } else {
                ++pendingSteps;
            }
        }
        REQUIRE(pendingSteps > 0);
        REQUIRE_EQUAL(
            el::String{result},
            "31415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"_el);
    }
};
