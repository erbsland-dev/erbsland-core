// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(ByteCount)
class LexerStandardByteCountTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testValidByteCounts() {
        // Simple valid usages
        WITH_CONTEXT(verifyValidInteger(R"(0kb)"_el, 0));
        WITH_CONTEXT(verifyValidInteger(R"(1 kb)"_el, 1000));
        WITH_CONTEXT(verifyValidInteger(R"(1 kib)"_el, 1024));
        WITH_CONTEXT(verifyValidInteger(R"(10 MB)"_el, 10000000));
        WITH_CONTEXT(verifyValidInteger(R"(10 MiB)"_el, 10 * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(999 gb)"_el, 999000000000ULL));
        WITH_CONTEXT(verifyValidInteger(R"(2 giB)"_el, 2ULL * 1024 * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(1 Tb)"_el, 1000000000000ULL));
        WITH_CONTEXT(verifyValidInteger(R"(5 tib)"_el, 5ULL * 1024 * 1024 * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(1 PB)"_el, 1000000000000000ULL));
        WITH_CONTEXT(verifyValidInteger(R"(2 PiB)"_el, 2ULL * 1024 * 1024 * 1024 * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(3 Eb)"_el, 1000000000000000000ULL * 3));
        WITH_CONTEXT(verifyValidInteger(R"(1 EiB)"_el, 1024ULL * 1024 * 1024 * 1024 * 1024 * 1024));
        // Suffixes and case
        WITH_CONTEXT(verifyValidInteger(R"(100 KiB)"_el, 100 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(900 kib)"_el, 900 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(1 KIB)"_el, 1024));
        WITH_CONTEXT(verifyValidInteger(R"(1 kb)"_el, 1000));
        WITH_CONTEXT(verifyValidInteger(R"(1 KB)"_el, 1000));
        // Test optional plus or minus sign and spaces
        WITH_CONTEXT(verifyValidInteger(R"(+123 mb)"_el, +123000000));
        WITH_CONTEXT(verifyValidInteger(R"(-3 Gb)"_el, -3000000000LL));
        WITH_CONTEXT(verifyValidInteger(R"(+1 MiB)"_el, +1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(-1 MiB)"_el, -1024 * 1024));
        // Test digit separators
        WITH_CONTEXT(verifyValidInteger(R"(10'000 mb)"_el, 10000000000ULL));
        WITH_CONTEXT(verifyValidInteger(R"(1'024 KiB)"_el, 1024 * 1024));
        // No space between number and suffix, and with space
        WITH_CONTEXT(verifyValidInteger(R"(42Gb)"_el, 42000000000ULL));
        WITH_CONTEXT(verifyValidInteger(R"(42 GiB)"_el, 42ULL * 1024 * 1024 * 1024));
        // Lowercase, uppercase, and mixed case accepted
        WITH_CONTEXT(verifyValidInteger(R"(7 mib)"_el, 7ULL * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(7 MiB)"_el, 7ULL * 1024 * 1024));
        WITH_CONTEXT(verifyValidInteger(R"(7 MiB)"_el, 7ULL * 1024 * 1024));
    }

    void testInvalidByteCounts() {
        // Out of range examples for 64-bit signed integers
        WITH_CONTEXT(verifyErrorInValue(
            R"(9223372036854775808 kb)"_el, ConfErrorCategory::LimitExceeded));            // decimal part too large
        WITH_CONTEXT(verifyErrorInValue(R"(1 yb)"_el, ConfErrorCategory::LimitExceeded));  // Exceeds 64-bit
        WITH_CONTEXT(verifyErrorInValue(R"(1 YiB)"_el, ConfErrorCategory::LimitExceeded)); // Exceeds 64-bit

        // Overflow when applying factor
        WITH_CONTEXT(verifyErrorInValue(R"(9223372036854777 kb)"_el, ConfErrorCategory::LimitExceeded));
        // 9223372036854777000 > max 64-bit
        WITH_CONTEXT(verifyErrorInValue(R"(9223372036854 GiB)"_el, ConfErrorCategory::LimitExceeded));
        // 9223372036854*1024^3 > max 64-bit

        // Too many digits (max 19 for 64-bit, ignoring separators)
        WITH_CONTEXT(verifyErrorInValue(R"(12345678901234567890 mb)"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue(R"(123456789012345678901 KiB)"_el, ConfErrorCategory::LimitExceeded));

        // Decimal integer rules: leading zeros
        WITH_CONTEXT(verifyErrorInValue(R"(0001 kb)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(01 KiB)"_el, ConfErrorCategory::Syntax));

        // Invalid digit separator: at start or end
        WITH_CONTEXT(verifyErrorInValue(R"('1000 mb)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(1000' mb)"_el, ConfErrorCategory::Syntax));
        // Consecutive digit separators
        WITH_CONTEXT(verifyErrorInValue(R"(100''000 kb)"_el, ConfErrorCategory::Syntax));

        // Unknown suffix or partial matches
        WITH_CONTEXT(verifyErrorInValue(R"(10 blabla)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(5 k)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(5 Mi)"_el, ConfErrorCategory::Syntax));
        // Space inside the suffix is not allowed
        WITH_CONTEXT(verifyErrorInValue(R"(5 Ki B)"_el, ConfErrorCategory::Syntax));
    }
};
