// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/EncodingErrorMode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>

using el::text::EncodingErrorMode;

TESTED_TARGETS(EncodingErrorMode)
class EncodingErrorModeTest final : public el::UnitTest {
public:
    void testStableValues() {
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EncodingErrorMode::Throw), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EncodingErrorMode::Ignore), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EncodingErrorMode::Replace), 2U);
    }
};
