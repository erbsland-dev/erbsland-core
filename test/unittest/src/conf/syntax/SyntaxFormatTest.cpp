// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/syntax/Format.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SyntaxFormat)
class SyntaxFormatTest final : public el::UnitTest {
public:
    void testComponentPatterns() {
        const auto document = el::conf::syntax::Parser{}.parseText("[Server]\nport: 1\n[server.deep]\nport: 2\n"_el);
        const auto shallow = el::conf::syntax::Selector{}.addPath("SERVER.*"_el);
        const auto recursive = el::conf::syntax::Selector{}.addPath("server.**.port"_el);
        REQUIRE(shallow.match(document->statements().getRef(el::ItemIndex{1})));
        REQUIRE_FALSE(shallow.match(document->statements().getRef(el::ItemIndex{3})));
        REQUIRE(recursive.match(document->statements().getRef(el::ItemIndex{1})));
        REQUIRE(recursive.match(document->statements().getRef(el::ItemIndex{3})));
        REQUIRE(
            el::conf::syntax::Selector{}
                .addPath("server.**"_el)
                .match(document->statements().getRef(el::ItemIndex{0})));
    }
    void testPerPropertySpecificityAndTies() {
        using namespace el::conf::syntax;
        const auto document = el::conf::syntax::Parser{}.parseText("[server]\nport: 3\n"_el);
        auto format = Format{};
        format.addRule({Selector{}.addPath("server.port"_el), FormatStylePatch{}.setValueNameCase(WordCase::Upper)});
        format.addRule(
            {Selector{}.addPath("server.*"_el),
                FormatStylePatch{}.setSeparator(el::Char{U'='}).setValueNameCase(WordCase::Lower)});
        auto style = format.resolve(document->statements().getRef(el::ItemIndex{1}));
        REQUIRE(style.valueNameCase() == WordCase::Upper);
        REQUIRE(style.separator() == U'=');
        format.addRule({Selector{}.addPath("server.port"_el), FormatStylePatch{}.setValueNameCase(WordCase::Title)});
        REQUIRE(format.resolve(document->statements().getRef(el::ItemIndex{1})).valueNameCase() == WordCase::Title);
    }
    void testQuotedComponentsAndUnresolvedPaths() {
        using namespace el::conf::syntax;
        const auto document = el::conf::syntax::Parser{}.parseText("[section.\"a.b\"]\nx: 3\n[bad\ny: 4\n"_el);
        REQUIRE(Selector{}.addPath("section.\"a.b\".x"_el).match(document->statements().getRef(el::ItemIndex{1})));
        REQUIRE(Selector{}.addPath("section.*.x"_el).match(document->statements().getRef(el::ItemIndex{1})));
        REQUIRE_FALSE(Selector{}.addPath("**"_el).match(document->statements().getRef(el::ItemIndex{3})));
    }
};
