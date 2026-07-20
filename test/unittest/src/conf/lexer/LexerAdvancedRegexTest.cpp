// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::re::RegExPtr)
class LexerAdvancedRegexTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testBasicRegex() {
        WITH_CONTEXT(verifyValidRegEx(R"(//)"_el, ""_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/text/)"_el, "text"_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/    text/)"_el, "    text"_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/text    /)"_el, "text    "_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/    te    xt    /)"_el, "    te    xt    "_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/😄➟←Æ×∃⚫︎/)"_el, "😄➟←Æ×∃⚫︎"_el));
    }

    void testEscapeSequences() {
        // Escape sequences must be passed to the backend, except `\/` that escapes `/`
        WITH_CONTEXT(verifyValidRegEx(R"(/text\n/)"_el, R"(text\n)"_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/\ntext/)"_el, R"(\ntext)"_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/\/text/)"_el, R"(/text)"_el));
        WITH_CONTEXT(verifyValidRegEx(R"(/text\//)"_el, R"(text/)"_el));
        WITH_CONTEXT(
            verifyValidRegEx(R"(/\a\b\c\d\e\f\gf\h\i\j\k\0\?\\\"/)"_el, R"(\a\b\c\d\e\f\gf\h\i\j\k\0\?\\\")"_el));
    }

    void testInvalidRegex() {
        WITH_CONTEXT(
            verifyErrorInValue(R"(/text\/)"_el, {ConfErrorCategory::UnexpectedEnd, ConfErrorCategory::Syntax}));
    }
};
