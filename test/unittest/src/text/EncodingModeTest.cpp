// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/EncodingMode.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>

using el::text::EncodingMode;

TESTED_TARGETS(EncodingMode)
class EncodingModeTest final : public el::UnitTest {
public:
    void testStableValues() {
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EncodingMode::Tolerant), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EncodingMode::Strict), 1U);
    }
};
