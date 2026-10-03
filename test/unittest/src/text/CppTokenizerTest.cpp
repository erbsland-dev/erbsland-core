// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(CppTokenizer)
class CppTokenizerTest final : public el::UnitTest {
public:
    void testNativeTokenPositions() {
        const auto source =
            el::String{"prefix// ä😀\r\nnamespace 名 { }\n"_el}.slice(el::StringSide::Back, el::ByteIndex{6});
        auto tokenizer = el::cpp::CppTokenizer{source};
        const auto comment = tokenizer.next();
        REQUIRE(comment.position().isZero());
        const auto keyword = tokenizer.next();
        REQUIRE_EQUAL(keyword.text(), "namespace"_el);
        REQUIRE_EQUAL(keyword.characterPosition(), el::CpIndex{7});
        REQUIRE_EQUAL(keyword.position(), el::ByteIndex{11});
        const auto name = tokenizer.next();
        REQUIRE_EQUAL(name.text(), "名"_el);
        REQUIRE_EQUAL(source.slice(el::ByteRange{name.position(), name.text().length()}), name.text());
        REQUIRE_EQUAL(tokenizer.next().position(), el::ByteIndex{25});
        REQUIRE_EQUAL(tokenizer.next().position(), el::ByteIndex{27});
        const auto end = tokenizer.next();
        REQUIRE(end.isEnd());
        REQUIRE_EQUAL(end.position(), el::ByteIndex::end(source.length()));
        REQUIRE_EQUAL(tokenizer.next().position(), end.position());
        REQUIRE(el::cpp::CppToken{}.position().isNoIndex());
    }

    void testRangesAcrossTokenKinds() {
        const auto source =
            el::String{"// ä😀\n#define 名 7\\\n + 2\nnamespace 名 { auto value = u8R\"(ä\n😀)\"; 123; }\n"_el};
        auto tokenizer = el::cpp::CppTokenizer{source};
        auto count = el::ItemCount::zero();
        while (true) {
            const auto token = tokenizer.next();
            REQUIRE_EQUAL(token.characterPosition(), token.characterRange().beginIndex());
            REQUIRE_EQUAL(token.position(), token.range().beginIndex());
            REQUIRE_EQUAL(token.characterRange().length(), token.text().characterLength());
            REQUIRE_EQUAL(token.range().length(), token.text().length());
            REQUIRE_EQUAL(token.characterLength(), token.characterRange().length());
            REQUIRE_EQUAL(token.length(), token.range().length());
            REQUIRE_EQUAL(source.slice(token.characterRange()), token.text());
            REQUIRE_EQUAL(source.slice(token.range()), token.text());
            if (token.isEnd()) {
                REQUIRE_EQUAL(token.characterRange(), el::CpRange::emptyAt(el::CpIndex::end(source.characterLength())));
                REQUIRE_EQUAL(token.range(), el::ByteRange::emptyAt(el::ByteIndex::end(source.length())));
                break;
            }
            ++count;
        }
        REQUIRE_EQUAL(count, el::ItemCount{13});
        REQUIRE_FALSE(el::cpp::CppToken{}.characterRange().isValid());
        REQUIRE_FALSE(el::cpp::CppToken{}.range().isValid());
        REQUIRE(el::cpp::CppToken{}.characterLength().isZero());
        REQUIRE(el::cpp::CppToken{}.length().isZero());
        auto malformed = el::cpp::CppTokenizer{el::String{el::unittest::th::stdStringFromHex("FF 61 20 C3 A4")}};
        const auto first = malformed.next();
        REQUIRE_EQUAL(first.characterRange(), (el::CpRange{el::CpIndex{0}, el::CpCount{2}}));
        REQUIRE_EQUAL(first.range(), (el::ByteRange{el::ByteIndex{0}, el::ByteCount{2}}));
        const auto second = malformed.next();
        REQUIRE_EQUAL(second.characterRange(), (el::CpRange{el::CpIndex{3}, el::CpCount{1}}));
        REQUIRE_EQUAL(second.range(), (el::ByteRange{el::ByteIndex{3}, el::ByteCount{2}}));
    }

    void testKindsAndSharedCaptures() {
        const auto source = "// comment\n#define DECL \\\r\n class Fake {};\n"
                            "namespace sample::net { class Real {}; }\n"
                            "u8R\"tag(class Hidden {}; // text)tag\" 1'000 0x1p+2 'x' \"a\\\"b\""_el;
        auto tokenizer = el::cpp::CppTokenizer{source};
        const auto comment = tokenizer.next();
        REQUIRE_EQUAL(comment.kind(), el::cpp::CppToken::Kind::Comment);
        REQUIRE_EQUAL(comment.text(), "// comment\n"_el);
        REQUIRE_EQUAL(comment.characterPosition(), el::CpIndex{0});
        const auto directive = tokenizer.next();
        REQUIRE_EQUAL(directive.kind(), el::cpp::CppToken::Kind::Directive);
        REQUIRE(directive.text().contains("class Fake"_el));
        auto significant = el::StringList{};
        auto literals = el::StringList{};
        while (true) {
            const auto token = tokenizer.next();
            if (token.kind() == el::cpp::CppToken::Kind::End) {
                break;
            }
            if (token.kind() == el::cpp::CppToken::Kind::Literal) {
                literals.append(token.text());
            } else {
                significant.append(token.text());
            }
        }
        REQUIRE_EQUAL(significant.join(" "_el), "namespace sample :: net { class Real { } ; } 1'000 0x1p+2"_el);
        REQUIRE_EQUAL(literals.count(), el::ItemCount{3});
        REQUIRE(literals.first().startsWith("u8R\"tag("_el));
        REQUIRE_EQUAL(comment.text(), "// comment\n"_el);
        REQUIRE_EQUAL(tokenizer.next().kind(), el::cpp::CppToken::Kind::End);
    }

    void testGeneratorLifetimeAndPunctuation() {
        auto stream = el::cpp::CppTokenizer::tokens("a::b >>= 2; x <=> y;"_el);
        auto tokens = el::StringList{};
        for (const auto &token : stream) {
            tokens.append(token.text());
        }
        REQUIRE_EQUAL(tokens.join(" "_el), "a :: b >>= 2 ; x <=> y ;"_el);
    }

    void testPunctuationDecisionBranches() {
        const auto punctuation = el::StringList{
            "<=>"_el,
            ">>="_el,
            "<<="_el,
            "->*"_el,
            "..."_el,
            "::"_el,
            "->"_el,
            ".*"_el,
            "++"_el,
            "--"_el,
            "<<"_el,
            ">>"_el,
            "<="_el,
            ">="_el,
            "=="_el,
            "!="_el,
            "&&"_el,
            "||"_el,
            "+="_el,
            "-="_el,
            "*="_el,
            "/="_el,
            "%="_el,
            "&="_el,
            "|="_el,
            "^="_el,
            "##"_el,
            ":"_el,
            "+"_el,
            "-"_el,
            "*"_el,
            "/"_el,
            "%"_el,
            "&"_el,
            "|"_el,
            "^"_el,
            "!"_el,
            "="_el,
            "<"_el,
            ">"_el,
            "."_el,
            "#"_el,
            "("_el,
            ")"_el,
            "{"_el,
            "}"_el,
            ";"_el};
        auto tokenizer = el::cpp::CppTokenizer{el::join("x "_el, punctuation.join(" "_el))};
        REQUIRE_EQUAL(tokenizer.next().text(), "x"_el);
        for (const auto &expected : punctuation) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto token = tokenizer.next();
                    REQUIRE_EQUAL(token.kind(), el::cpp::CppToken::Kind::Punctuation);
                    REQUIRE_EQUAL(token.text(), expected);
                },
                [&]() -> std::string { return "punctuation branch"; });
        }
        REQUIRE(tokenizer.next().isEnd());
        WITH_CONTEXT(requireTokenTexts("..x .... <== >=> ->x /x ::x"_el, ". . x ... . <= = >= > -> x / x :: x"_el));
        WITH_CONTEXT(requireTokenTexts(".."_el, ". ."_el));
        WITH_CONTEXT(requireTokenTexts("1e+2 0x1p-2 1'000 2.5 .5"_el, "1e+2 0x1p-2 1'000 2.5 . 5"_el));
    }

    void testLiteralPrefixBranches() {
        for (const auto &prefix : {""_el, "u8"_el, "u"_el, "U"_el, "L"_el}) {
            for (const auto &literal : {"\"text\""_el, "'x'"_el, "R\"tag(raw )near\" text)tag\""_el}) {
                const auto source = el::join(prefix, literal);
                auto tokenizer = el::cpp::CppTokenizer{source};
                const auto token = tokenizer.next();
                REQUIRE_EQUAL(token.kind(), el::cpp::CppToken::Kind::Literal);
                REQUIRE_EQUAL(token.text(), source);
                REQUIRE(tokenizer.next().isEnd());
            }
        }
        WITH_CONTEXT(
            requireTokenTexts("R u u8 U L u8R raw RR u8name R'x'"_el, "R u u8 U L u8R raw RR u8name R 'x'"_el));
        WITH_CONTEXT(requireTokenTexts(
            "R\"1234567890123456(ok)1234567890123456\""_el, "R\"1234567890123456(ok)1234567890123456\""_el));
        auto invalid = el::cpp::CppTokenizer{"R\"12345678901234567(text)12345678901234567\""_el};
        REQUIRE_THROWS_AS(el::ParseError, invalid.next());
    }

    void testLineAndBlockCommentBranches() {
        auto tokenizer = el::cpp::CppTokenizer{"// one\\\r\n two\\\n three\n/* **/ #define X \\\n 1\nname / value"_el};
        REQUIRE_EQUAL(tokenizer.next().text(), "// one\\\r\n two\\\n three\n"_el);
        REQUIRE_EQUAL(tokenizer.next().text(), "/* **/"_el);
        REQUIRE_EQUAL(tokenizer.next().kind(), el::cpp::CppToken::Kind::Directive);
        REQUIRE_EQUAL(tokenizer.next().text(), "name"_el);
        REQUIRE_EQUAL(tokenizer.next().text(), "/"_el);
        REQUIRE_EQUAL(tokenizer.next().text(), "value"_el);
        REQUIRE(tokenizer.next().isEnd());
        WITH_CONTEXT(requireTokenTexts("// last\\\r"_el, "// last\\\r"_el));
        WITH_CONTEXT(requireTokenTexts("\"a\\\r\nb\" 'c\\\nd'"_el, "\"a\\\r\nb\" 'c\\\nd'"_el));
    }

    void testCommentsAndDirectiveBoundaries() {
        auto tokens = el::StringList{};
        for (
            const auto &token :
            el::cpp::CppTokenizer::tokens("/*leading*/ #if 1\n x # y // continued\\\n class Hidden {};\n z"_el)) {
            if (token.kind() != el::cpp::CppToken::Kind::Comment &&
                token.kind() != el::cpp::CppToken::Kind::Directive) {
                tokens.append(token.text());
            }
        }
        REQUIRE_EQUAL(tokens.join(" "_el), "x # y z"_el);
    }

    void testMalformedInput() {
        for (const auto &source : {"/* unclosed"_el, "\"unclosed"_el, "R\"tag(unclosed"_el, "R\"bad delimiter("_el}) {
            auto tokenizer = el::cpp::CppTokenizer{source};
            REQUIRE_THROWS_AS(el::ParseError, tokenizer.next());
        }
    }
    void testOffsetsAndRetainedTokens() {
        auto retained = el::cpp::CppToken{};
        REQUIRE(retained.isEnd());
        {
            auto tokenizer = el::cpp::CppTokenizer{"ä β;"_el};
            retained = tokenizer.next();
            REQUIRE_EQUAL(retained.characterPosition(), el::CpIndex{0});
            const auto second = tokenizer.next();
            REQUIRE_EQUAL(second.characterPosition(), el::CpIndex{2});
            REQUIRE_EQUAL(second.text(), "β"_el);
            REQUIRE_EQUAL(tokenizer.next().text(), ";"_el);
            const auto end = tokenizer.next();
            REQUIRE(end.isEnd());
            REQUIRE(end.text().isEmpty());
            REQUIRE_EQUAL(end.characterPosition(), el::CpIndex{4});
            REQUIRE_EQUAL(tokenizer.next().characterPosition(), end.characterPosition());
        }
        REQUIRE_EQUAL(retained.text(), "ä"_el);
    }

    void testLazyErrorsAndIndependentCursors() {
        auto tokenizer = el::cpp::CppTokenizer{"first second"_el};
        auto copied = tokenizer;
        REQUIRE_EQUAL(tokenizer.next().text(), "first"_el);
        REQUIRE_EQUAL(tokenizer.next().text(), "second"_el);
        REQUIRE_EQUAL(copied.next().text(), "first"_el);
        auto stream = el::cpp::CppTokenizer::tokens("/*"_el);
        REQUIRE_THROWS_AS(el::ParseError, stream.begin());
        auto malformed = el::cpp::CppTokenizer{"ä /*"_el};
        REQUIRE_EQUAL(malformed.next().text(), "ä"_el);
        try {
            [[maybe_unused]] const auto token = malformed.next();
            REQUIRE(false);
        } catch (const el::ParseError &error) {
            REQUIRE_EQUAL(error.codePointIndex(), el::CpIndex{4});
        }
    }

private:
    void requireTokenTexts(const el::String &source, const el::String &expected) {
        auto tokens = el::StringList{};
        for (const auto &token : el::cpp::CppTokenizer::tokens(source)) {
            tokens.append(token.text());
        }
        REQUIRE_EQUAL(tokens.join(" "_el), expected);
    }
};
