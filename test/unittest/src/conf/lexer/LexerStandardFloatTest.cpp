// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Float)
class LexerStandardFloatTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testValidFloat() {
        WITH_CONTEXT(verifyValidFloat(R"(0.0)"_el, 0.0));
        WITH_CONTEXT(verifyValidFloat(R"(.0)"_el, 0.0));
        WITH_CONTEXT(verifyValidFloat(R"(0.)"_el, 0.0));
        WITH_CONTEXT(verifyValidFloat(R"(+0.)"_el, 0.0));
        WITH_CONTEXT(verifyValidFloat(R"(-0.0)"_el, -0.0));
        WITH_CONTEXT(verifyValidFloat(R"(1.0)"_el, 1.0));
        WITH_CONTEXT(verifyValidFloat(R"(-1.0)"_el, -1.0));
        WITH_CONTEXT(verifyValidFloat(R"(12345.6789)"_el, 12345.6789));
        WITH_CONTEXT(verifyValidFloat(R"(0.0000000000000001)"_el, 1e-16));
        WITH_CONTEXT(verifyValidFloat(R"(1e0)"_el, 1e0));
        WITH_CONTEXT(verifyValidFloat(R"(1E+10)"_el, 1e10));
        WITH_CONTEXT(verifyValidFloat(R"(1E-5)"_el, 1e-5));
        WITH_CONTEXT(verifyValidFloat(R"(12.34e56)"_el, 12.34e56));
        WITH_CONTEXT(verifyValidFloat(R"(10000000000e-000005)"_el, 10000000000.0e-5));
        WITH_CONTEXT(verifyValidFloat(R"(8'283.9e-5)"_el, 8283.9e-5));
        WITH_CONTEXT(verifyValidFloat(R"(100'000.000'001)"_el, 100000.000001));
        WITH_CONTEXT(verifyValidFloat(R"(nan)"_el, std::numeric_limits<double>::quiet_NaN()));
        WITH_CONTEXT(verifyValidFloat(R"(+NaN)"_el, std::numeric_limits<double>::quiet_NaN()));
        WITH_CONTEXT(verifyValidFloat(R"(-NaN)"_el, -std::numeric_limits<double>::quiet_NaN()));
        WITH_CONTEXT(verifyValidFloat(R"(inf)"_el, std::numeric_limits<double>::infinity()));
        WITH_CONTEXT(verifyValidFloat(R"(+INF)"_el, std::numeric_limits<double>::infinity()));
        WITH_CONTEXT(verifyValidFloat(R"(-inf)"_el, -std::numeric_limits<double>::infinity()));
    }

    void testInvalidFloat() {
        WITH_CONTEXT(verifyErrorInValue(R"(005.293)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(10000000000.00000000001)"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue(R"(1.000000000000000000000)"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue(R"(12.3.4)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(1.23e1234567)"_el, ConfErrorCategory::LimitExceeded));
        WITH_CONTEXT(verifyErrorInValue(R"(0x1.23p+1)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"('100'000.0)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(100'000'.0)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(100''000.0)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"(0.'100'000)"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(
            verifyErrorInValue(R"(0.100'000')"_el, {ConfErrorCategory::Syntax, ConfErrorCategory::UnexpectedEnd}));
        WITH_CONTEXT(verifyErrorInValue(R"(0.100''000)"_el, ConfErrorCategory::Syntax));
    }
};
