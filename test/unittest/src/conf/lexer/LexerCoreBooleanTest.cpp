// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Boolean)
class LexerCoreBooleanTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testBoolean() {
        // valid values.
        WITH_CONTEXT(verifyValidBoolean("true"_el, true));
        WITH_CONTEXT(verifyValidBoolean("yes"_el, true));
        WITH_CONTEXT(verifyValidBoolean("enabled"_el, true));
        WITH_CONTEXT(verifyValidBoolean("on"_el, true));
        WITH_CONTEXT(verifyValidBoolean("false"_el, false));
        WITH_CONTEXT(verifyValidBoolean("no"_el, false));
        WITH_CONTEXT(verifyValidBoolean("disabled"_el, false));
        WITH_CONTEXT(verifyValidBoolean("off"_el, false));

        WITH_CONTEXT(verifyValidBoolean("TRUE"_el, true));
        WITH_CONTEXT(verifyValidBoolean("YES"_el, true));
        WITH_CONTEXT(verifyValidBoolean("ENABLED"_el, true));
        WITH_CONTEXT(verifyValidBoolean("ON"_el, true));
        WITH_CONTEXT(verifyValidBoolean("FALSE"_el, false));
        WITH_CONTEXT(verifyValidBoolean("NO"_el, false));
        WITH_CONTEXT(verifyValidBoolean("DISABLED"_el, false));
        WITH_CONTEXT(verifyValidBoolean("OFF"_el, false));

        WITH_CONTEXT(verifyValidBoolean("TrUe"_el, true));
        WITH_CONTEXT(verifyValidBoolean("YeS"_el, true));
        WITH_CONTEXT(verifyValidBoolean("EnAbLed"_el, true));
        WITH_CONTEXT(verifyValidBoolean("On"_el, true));
        WITH_CONTEXT(verifyValidBoolean("FaLsE"_el, false));
        WITH_CONTEXT(verifyValidBoolean("No"_el, false));
        WITH_CONTEXT(verifyValidBoolean("DiSaBlEd"_el, false));
        WITH_CONTEXT(verifyValidBoolean("OfF"_el, false));
    }

    void testInvalidBoolean() {
        WITH_CONTEXT(verifyErrorInValue("truee"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("true0"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("false'"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("tru"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("fals"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("tr"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("fa"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("t"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("f"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue("tru"_el, ConfErrorCategory::Syntax));
    }
};
