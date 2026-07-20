// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Code)
class LexerStandardCodeTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testBasicCode() {
        WITH_CONTEXT(verifyValidCode(R"(``)"_el, ""_el));
        WITH_CONTEXT(verifyValidCode(R"(`text`)"_el, "text"_el));
        WITH_CONTEXT(verifyValidCode(R"(`    text`)"_el, "    text"_el));
        WITH_CONTEXT(verifyValidCode(R"(`text    `)"_el, "text    "_el));
        WITH_CONTEXT(verifyValidCode(R"(`    te    xt    `)"_el, "    te    xt    "_el));
        WITH_CONTEXT(verifyValidCode(R"(` \tte\t xt \t`)"_el, R"( \tte\t xt \t)"_el));
        WITH_CONTEXT(verifyValidCode(R"(`😄➟←Æ×∃⚫︎`)"_el, "😄➟←Æ×∃⚫︎"_el));
        WITH_CONTEXT(verifyValidCode(R"(`\`)"_el, R"(\)"_el));
        WITH_CONTEXT(verifyValidCode(R"(`""`)"_el, R"("")"_el));
        WITH_CONTEXT(verifyValidCode(R"(`\n\t\u0020\u{20}\$\"`)"_el, R"(\n\t\u0020\u{20}\$\")"_el));
    }
};
