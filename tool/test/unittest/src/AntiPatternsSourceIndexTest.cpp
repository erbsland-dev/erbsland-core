// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CpCount.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/SourceIndex.hpp>

using namespace el::text::literals;

TESTED_TARGETS(tool::anti_patterns::SourceIndex tool::anti_patterns::SourceBlock)
class AntiPatternsSourceIndexTest final : public el::UnitTest {
public:
    void testPhysicalLinesAndEof() {
        const auto empty = tool::anti_patterns::SourceIndex{el::Path{"Empty.cpp"_el}, {}};
        REQUIRE_EQUAL(empty.lineCount(), el::LineCount{1});
        REQUIRE_EQUAL(empty.lineIndex(el::ByteIndex::zero()), el::LineIndex::zero());
        REQUIRE_EQUAL(empty.lineStart(el::LineIndex::zero()), el::ByteIndex::zero());
        REQUIRE_EQUAL(empty.lineEnd(el::LineIndex::zero()), el::ByteIndex::zero());
        REQUIRE(empty.line(el::LineIndex::zero()).isEmpty());

        for (const auto trailingNewline : {false, true}) {
            auto text = "ä\r\nΩ\nx"_el;
            if (trailingNewline) {
                text = "ä\r\nΩ\nx\n"_el;
            }
            const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, text};
            REQUIRE_EQUAL(source.lineCount(), el::LineCount{3});
            REQUIRE_EQUAL(source.line(el::LineIndex{0}), "ä\r"_el);
            REQUIRE_EQUAL(source.line(el::LineIndex{1}), "Ω"_el);
            REQUIRE_EQUAL(source.line(el::LineIndex{2}), "x"_el);
            REQUIRE_EQUAL(source.lineStart(el::LineIndex{1}), el::ByteIndex{4});
            REQUIRE_EQUAL(source.lineEnd(el::LineIndex{1}), el::ByteIndex{6});
            REQUIRE_EQUAL(source.lineIndex(el::ByteIndex{3}), el::LineIndex{0});
            REQUIRE_EQUAL(source.lineIndex(el::ByteIndex{4}), el::LineIndex{1});
            REQUIRE_EQUAL(source.lineIndex(el::ByteIndex{6}), el::LineIndex{1});
            REQUIRE_EQUAL(source.lineIndex(el::ByteIndex{7}), el::LineIndex{2});
            const auto eof = el::ByteIndex::end(source.text().length());
            const auto lastLine = trailingNewline ? el::LineIndex{3} : el::LineIndex{2};
            REQUIRE_EQUAL(source.lineIndex(eof), lastLine);
            REQUIRE_EQUAL(source.lineStart(el::LineIndex{9}), eof);
            REQUIRE_EQUAL(source.lineEnd(el::LineIndex{9}), eof);
            REQUIRE(source.line(el::LineIndex{9}).isEmpty());
        }
    }

    void testMaskAndFindingByteCoordinates() {
        const auto text = el::String{"// ä🙂\r\nconst auto text = \"Ω\";\nnamespace { }\n"_el};
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, text};
        REQUIRE_EQUAL(source.masked().length(), source.text().length());
        auto position = el::ByteIndex::zero();
        while (true) {
            const auto newline = text.find("\n"_el, position);
            REQUIRE_EQUAL(source.masked().find("\n"_el, position), newline);
            if (newline.isNoIndex()) {
                break;
            }
            position = newline.incremented();
        }
        REQUIRE_FALSE(source.masked().contains("ä"_el));
        REQUIRE_FALSE(source.masked().contains("Ω"_el));
        const auto registry = tool::anti_patterns::RuleRegistry{};
        auto candidates = el::List<el::ByteRange>{};
        registry.find("anonymous_namespace"_el)->scan(source, candidates);
        REQUIRE_EQUAL(candidates.count(), el::ItemCount{1});
        const auto expected = el::ByteRange{text.find("namespace"_el), text.find("{"_el).incremented()};
        REQUIRE_EQUAL(candidates.first(), expected);
        REQUIRE_EQUAL(source.lineIndex(expected.beginIndex()), el::LineIndex{2});
        REQUIRE_EQUAL(source.snippet(expected), "namespace {"_el);
    }

    void testSnippetWhitespaceAndEmbeddedNull() {
        const auto text = el::join(
            " \talpha\r\n\0\u00a0 beta\v\f"_el, el::String{el::unittest::th::stdStringFromHex("ff")}, " \t"_el);
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, text};
        REQUIRE_EQUAL(source.snippet({el::ByteIndex::zero(), text.length()}), "alpha \0\u00a0 beta �"_el);

        const auto whitespace = el::String{" \t\r\n\v\f"_el};
        const auto empty = tool::anti_patterns::SourceIndex{el::Path{"Empty.cpp"_el}, whitespace};
        REQUIRE(empty.snippet({el::ByteIndex::zero(), whitespace.length()}).isEmpty());
    }

    void testSnippetTruncationCountsCodePoints() {
        const auto prefix = el::String::fromCharacter(U'ä', el::CpCount{94});
        const auto text = el::join(prefix, " \0more"_el);
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, text};
        const auto snippet = source.snippet({el::ByteIndex::zero(), text.length()});
        REQUIRE_EQUAL(snippet, el::join(prefix, " …"_el));
        REQUIRE_EQUAL(snippet.characterLength(), el::CpCount{96});

        const auto exact = el::join(prefix, "\0x"_el);
        const auto exactSource = tool::anti_patterns::SourceIndex{el::Path{"Exact.cpp"_el}, exact};
        REQUIRE_EQUAL(exactSource.snippet({el::ByteIndex::zero(), exact.length()}), exact);
    }

    void testBlockParentsAndUnclosedScopes() {
        const auto text = el::String{"namespace a { class B { void f() { call(); } }; }"_el};
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.hpp"_el}, text};
        REQUIRE_EQUAL(source.blocks().count(), el::ItemCount{3});
        const auto &outer = source.blocks().getRef(el::ItemIndex{0});
        const auto &type = source.blocks().getRef(el::ItemIndex{1});
        const auto &function = source.blocks().getRef(el::ItemIndex{2});
        REQUIRE(outer.parent.isNoIndex());
        REQUIRE(source.parent(outer) == nullptr);
        REQUIRE_EQUAL(type.parent, el::ItemIndex{0});
        REQUIRE_EQUAL(function.parent, el::ItemIndex{1});
        REQUIRE(source.parent(function) == &type);
        REQUIRE_EQUAL(source.enclosingBlock(text.find("call"_el)), el::ItemIndex{2});
        REQUIRE(source.enclosingBlock(outer.openingBrace()).isNoIndex());
        REQUIRE(source.enclosingBlock(outer.closingBrace()).isNoIndex());
        REQUIRE_FALSE(source.namespaceScope(text.find("call"_el)));
        REQUIRE_EQUAL(outer.braces.endIndex(), el::ByteIndex::end(text.length()));

        const auto unclosed = tool::anti_patterns::SourceIndex{el::Path{"Example.hpp"_el}, "class B { int x;"_el};
        const auto &block = unclosed.blocks().getRef(el::ItemIndex{0});
        const auto eof = el::ByteIndex::end(unclosed.text().length());
        REQUIRE_EQUAL(block.closingBrace(), eof);
        REQUIRE_EQUAL(block.braces.endIndex(), eof.incremented());
        REQUIRE_EQUAL(unclosed.enclosingBlock(unclosed.text().find("int"_el)), el::ItemIndex{0});
        REQUIRE_EQUAL(unclosed.slice(block.braces), "{ int x;"_el);
    }

    void testClampedSpansAndInlineAcceptance() {
        const auto source = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, "abc\n"_el};
        REQUIRE_EQUAL(source.slice({el::ByteIndex{1}, el::ByteCount{99}}), "bc\n"_el);
        REQUIRE(source.slice({el::ByteIndex{99}, el::ByteCount{1}}).isEmpty());
        REQUIRE(source.slice(el::ByteRange::emptyAt(el::ByteIndex::zero())).isEmpty());
        REQUIRE(source.inlineReason("anonymous_namespace"_el, el::ByteRange::empty()).isEmpty());
        REQUIRE_EQUAL(source.statementEnd(el::ByteIndex::zero()), el::ByteIndex{3});

        const auto text = el::String{"// anti-pattern: allow anonymous_namespace -- ä🙂\nnamespace { }\n"_el};
        const auto accepted = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, text};
        const auto range = el::ByteRange{text.find("namespace {"_el), text.find("{"_el).incremented()};
        REQUIRE_EQUAL(accepted.inlineReason("anonymous_namespace"_el, range), "ä🙂"_el);
        const auto inlineText = el::String{"namespace { } // anti-pattern: allow anonymous_namespace -- Local.\n"_el};
        const auto inlineSource = tool::anti_patterns::SourceIndex{el::Path{"Example.cpp"_el}, inlineText};
        REQUIRE_EQUAL(
            inlineSource.inlineReason(
                "anonymous_namespace"_el, el::ByteRange{el::ByteIndex::zero(), inlineText.find("}"_el).incremented()}),
            "Local."_el);
    }
};
