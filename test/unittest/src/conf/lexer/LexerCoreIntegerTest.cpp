// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Integer)
class LexerCoreIntegerTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testDecimalInteger() {
        // valid values.
        WITH_CONTEXT(verifyValidInteger("0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("-0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("1"_el, 1));
        WITH_CONTEXT(verifyValidInteger("-1"_el, -1));
        WITH_CONTEXT(verifyValidInteger("1234567890"_el, 1'234'567'890));
        WITH_CONTEXT(verifyValidInteger("-5'239"_el, -5'239));
        WITH_CONTEXT(verifyValidInteger("-9223372036854775808"_el, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(verifyValidInteger("9223372036854775807"_el, std::numeric_limits<int64_t>::max()));
        WITH_CONTEXT(verifyValidInteger("-9'223'372'036'854'775'808"_el, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(verifyValidInteger("9'223'372'036'854'775'807"_el, std::numeric_limits<int64_t>::max()));

        // limits exceeded
        WITH_CONTEXT(verifyErrorInValue("-9223372036854775809"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue("9223372036854775808"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue("-1000000000000000000000"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue("1000000000000000000000"_el, ConfErrorCategory::LimitExceeded));

        // wrong syntax to prevent confusing with octal-values.
        WITH_CONTEXT(verifyErrorInValue("00"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("01"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("-00"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("-01"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("02938"_el, ConfErrorCategory::Syntax));

        // Problems with digit separators
        WITH_CONTEXT(verifyErrorInValue("'123"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("123'"_el, {ConfErrorCategory::Syntax, ConfErrorCategory::UnexpectedEnd}));
        WITH_CONTEXT(verifyErrorInValue("1''23"_el, ConfErrorCategory::Syntax));
    }

    void testHexadecimalInteger() {
        // valid values.
        WITH_CONTEXT(verifyValidInteger("0x0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0x00"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0x0000000000000000"_el, 0));
        WITH_CONTEXT(verifyValidInteger("-0x0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0x1"_el, 1));
        WITH_CONTEXT(verifyValidInteger("0xa"_el, 0xa));
        WITH_CONTEXT(verifyValidInteger("0x0123456789abcdef"_el, 0x0123456789abcdef));
        WITH_CONTEXT(verifyValidInteger("0x0123456789ABCDEF"_el, 0x0123456789abcdef));
        WITH_CONTEXT(verifyValidInteger("-0x0123456789abcdef"_el, -0x0123456789abcdef));
        WITH_CONTEXT(verifyValidInteger("-0x0123456789ABCDEF"_el, -0x0123456789abcdef));
        WITH_CONTEXT(verifyValidInteger("0x0123'4567'89ab'cdef"_el, 0x0123456789abcdef));
        WITH_CONTEXT(verifyValidInteger("-0x8000000000000000"_el, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(verifyValidInteger("0x7fffffffffffffff"_el, std::numeric_limits<int64_t>::max()));

        // limits exceeded
        WITH_CONTEXT(verifyErrorInValue("0x00000000000000000000000000000000"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue("-0x8000000000000001"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue("0x8000000000000000"_el, ConfErrorCategory::LimitExceeded));

        // syntax problems
        WITH_CONTEXT(verifyErrorInValue("0xabcdefg"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("0x'0000"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("0x0000'"_el, {ConfErrorCategory::Syntax, ConfErrorCategory::UnexpectedEnd}));
        WITH_CONTEXT(verifyErrorInValue("0x00''00"_el, ConfErrorCategory::Syntax));
    }

    void testBinaryInteger() {
        // valid values.
        WITH_CONTEXT(verifyValidInteger("0b0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0b00"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0b0000000000000000"_el, 0));
        WITH_CONTEXT(verifyValidInteger("-0b0"_el, 0));
        WITH_CONTEXT(verifyValidInteger("0b1"_el, 1));
        WITH_CONTEXT(verifyValidInteger("0b10"_el, 2));
        WITH_CONTEXT(verifyValidInteger(
            "-0b1000000000000000000000000000000000000000000000000000000000000000"_el,
            std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(verifyValidInteger(
            "0b0111111111111111111111111111111111111111111111111111111111111111"_el,
            std::numeric_limits<int64_t>::max()));
        WITH_CONTEXT(verifyValidInteger("-0b1010'1000'1111'0010"_el, -0b1010'1000'1111'0010));

        // limits exceeded
        WITH_CONTEXT(verifyErrorInValue(
            "0b1000000000000000000000000000000000000000000000000000000000000000"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue(
            "-0b1000000000000000000000000000000000000000000000000000000000000001"_el,
            ConfErrorCategory::LimitExceeded));

        // syntax problems.
        WITH_CONTEXT(verifyErrorInValue("0b102"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("0b'0000"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("0b0000'"_el, {ConfErrorCategory::Syntax, ConfErrorCategory::UnexpectedEnd}));
        WITH_CONTEXT(verifyErrorInValue("0b00''00"_el, ConfErrorCategory::Syntax));
    }
};
