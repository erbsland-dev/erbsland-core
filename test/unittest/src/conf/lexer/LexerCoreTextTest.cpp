// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Text)
class LexerCoreTextTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testBasicText() {
        WITH_CONTEXT(verifyValidText(R"("")"_el, ""_el));
        WITH_CONTEXT(verifyValidText(R"("text")"_el, "text"_el));
        WITH_CONTEXT(verifyValidText(R"("    text")"_el, "    text"_el));
        WITH_CONTEXT(verifyValidText(R"("text    ")"_el, "text    "_el));
        WITH_CONTEXT(verifyValidText(R"("    te    xt    ")"_el, "    te    xt    "_el));
        WITH_CONTEXT(verifyValidText(R"(" \tte\t xt \t")"_el, " \tte\t xt \t"_el));
        WITH_CONTEXT(verifyValidText(R"("😄➟←Æ×∃⚫︎")"_el, "😄➟←Æ×∃⚫︎"_el));
    }

    void testEscapeSequences() {
        // test escape locations
        WITH_CONTEXT(verifyValidText("\"text\\n\""_el, "text\n"_el));
        WITH_CONTEXT(verifyValidText("\"\\ntext\""_el, "\ntext"_el));
        WITH_CONTEXT(verifyValidText("\"te\\nxt\""_el, "te\nxt"_el));
        WITH_CONTEXT(verifyValidText("\"\\\\\\\\te\\\\\\\\xt\\\\\\\\\""_el, "\\\\te\\\\xt\\\\"_el));
        WITH_CONTEXT(verifyValidText("\"\\\"\\\"te\\\"\\\"xt\\\"\\\"\""_el, "\"\"te\"\"xt\"\""_el));
        WITH_CONTEXT(verifyValidText("\"\\$\\$te\\$\\$xt\\$\\$\""_el, "$$te$$xt$$"_el));
        WITH_CONTEXT(verifyValidText("\"\\n\\nte\\n\\nxt\\n\\n\""_el, "\n\nte\n\nxt\n\n"_el));
        WITH_CONTEXT(verifyValidText("\"\\N\\Nte\\N\\Nxt\\N\\N\""_el, "\n\nte\n\nxt\n\n"_el));
        WITH_CONTEXT(verifyValidText("\"\\r\\Rte\\r\\rxt\\r\\r\""_el, "\r\rte\r\rxt\r\r"_el));
        WITH_CONTEXT(verifyValidText("\"\\t\\Tte\\t\\txt\\t\\t\""_el, "\t\tte\t\txt\t\t"_el));
        WITH_CONTEXT(verifyValidText("\"\\u0020\\U0020te\\u0020\\u0020xt\\u0020\\u0020\""_el, "  te  xt  "_el));
        WITH_CONTEXT(verifyValidText("\"\\u{20}\\U{20}te\\u{20}\\u{20}xt\\u{20}\\u{20}\""_el, "  te  xt  "_el));

        // test range checks
        WITH_CONTEXT(verifyValidText(R"("\u{a}")"_el, "\n"_el));
        WITH_CONTEXT(verifyValidText(R"("\u{20}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{0020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{25cf}")"_el, "●"_el));
        WITH_CONTEXT(verifyValidText(R"("\u{25CF}")"_el, "●"_el));
        WITH_CONTEXT(verifyValidText(R"("\U{25cf}")"_el, "●"_el));
        WITH_CONTEXT(verifyValidText(R"("\U{25CF}")"_el, "●"_el));
        WITH_CONTEXT(verifyValidText(R"("\u{00020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{000020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{0000020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{00000020}")"_el, " "_el));
        WITH_CONTEXT(verifyValidText(R"("\u{0010ffff}")"_el, "\U0010ffff"_el));

        // test errors
        WITH_CONTEXT(verifyErrorInValue(R"("\u{}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{0}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{00}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{d800}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{feff}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{ffffffff}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u{00110000}")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\ua")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u0a")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u00a")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\uatext")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u0atext")"_el, ConfErrorCategory::Syntax));
        WITH_CONTEXT(verifyErrorInValue(R"("\u00atext")"_el, ConfErrorCategory::Syntax));
    }

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
