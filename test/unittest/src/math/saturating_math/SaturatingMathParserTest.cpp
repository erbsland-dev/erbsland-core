// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SaturatingMathTestBase.hpp"

TESTED_TARGETS(SaturatingMath)
class SaturatingMathParserTest final : public UNITTEST_SUBCLASS(SaturatingMathTestBase) {
public:
    void testSignedEdgeValues() {
        REQUIRE_EQUAL(parseInteger<std::int8_t>("-128"), std::numeric_limits<std::int8_t>::min());
        REQUIRE_EQUAL(parseInteger<std::int8_t>("127"), std::numeric_limits<std::int8_t>::max());
        REQUIRE_EQUAL(parseInteger<std::int16_t>("-32768"), std::numeric_limits<std::int16_t>::min());
        REQUIRE_EQUAL(parseInteger<std::int16_t>("32767"), std::numeric_limits<std::int16_t>::max());
        REQUIRE_EQUAL(parseInteger<std::int32_t>("-2147483648"), std::numeric_limits<std::int32_t>::min());
        REQUIRE_EQUAL(parseInteger<std::int32_t>("2147483647"), std::numeric_limits<std::int32_t>::max());
        REQUIRE_EQUAL(parseInteger<std::int64_t>("-9223372036854775808"), std::numeric_limits<std::int64_t>::min());
        REQUIRE_EQUAL(parseInteger<std::int64_t>("9223372036854775807"), std::numeric_limits<std::int64_t>::max());
    }

    void testUnsignedEdgeValues() {
        REQUIRE_EQUAL(parseInteger<std::uint8_t>("0"), std::numeric_limits<std::uint8_t>::min());
        REQUIRE_EQUAL(parseInteger<std::uint8_t>("255"), std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQUAL(parseInteger<std::uint16_t>("0"), std::numeric_limits<std::uint16_t>::min());
        REQUIRE_EQUAL(parseInteger<std::uint16_t>("65535"), std::numeric_limits<std::uint16_t>::max());
        REQUIRE_EQUAL(parseInteger<std::uint32_t>("0"), std::numeric_limits<std::uint32_t>::min());
        REQUIRE_EQUAL(parseInteger<std::uint32_t>("4294967295"), std::numeric_limits<std::uint32_t>::max());
        REQUIRE_EQUAL(parseInteger<std::uint64_t>("0"), std::numeric_limits<std::uint64_t>::min());
        REQUIRE_EQUAL(parseInteger<std::uint64_t>("18446744073709551615"), std::numeric_limits<std::uint64_t>::max());
    }

    void testRejectedIntegerValues() {
        REQUIRE_THROWS(parseInteger<std::int8_t>(""));
        REQUIRE_THROWS(parseInteger<std::uint8_t>("-1"));
        REQUIRE_THROWS(parseInteger<std::uint8_t>("+1"));
        REQUIRE_THROWS(parseInteger<std::int8_t>("+1"));
        REQUIRE_THROWS(parseInteger<std::int8_t>("128"));
        REQUIRE_THROWS(parseInteger<std::uint8_t>("256"));
        REQUIRE_THROWS(parseInteger<std::int64_t>("-9223372036854775809"));
        REQUIRE_THROWS(parseInteger<std::uint64_t>("18446744073709551616"));
        REQUIRE_THROWS(parseInteger<std::int32_t>("12x"));
        REQUIRE_THROWS(parseInteger<std::int32_t>("x12"));
    }

    void testRejectedTypeAndBoolValues() {
        REQUIRE_THROWS(parseType("i128"));
        REQUIRE_THROWS(parseType(""));
        REQUIRE_THROWS(parseBool(""));
        REQUIRE_THROWS(parseBool("2"));
        REQUIRE_THROWS(parseBool("true"));
    }

    void testRejectedRows() {
        REQUIRE_THROWS(parseCastRow("", 1));
        REQUIRE_THROWS(parseCastRow("i8 i8 0 0", 1));
        REQUIRE_THROWS(parseCastRow("i8 i8 0 0 0 extra", 1));
        REQUIRE_THROWS(parseCastRow("i128 i8 0 0 0", 1));
        REQUIRE_THROWS(parseCastRow("i8 i8 0 0 x", 1));
        REQUIRE_THROWS(parseArithmeticRow("", 1));
        REQUIRE_THROWS(parseArithmeticRow("i8 i8 0 0 0", 1));
        REQUIRE_THROWS(parseArithmeticRow("i8 i8 0 0 0 0 extra", 1));
        REQUIRE_THROWS(parseArithmeticRow("i8 u128 0 0 0 0", 1));
        REQUIRE_THROWS(parseArithmeticRow("i8 i8 0 0 0 x", 1));
    }
};
