// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/html/HtmlParser.hpp>
#include <erbsland/text/impl/LinkData.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/text/TextNode.hpp>
#include <erbsland/text/TextNodeData.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using namespace el::text::literals;

using el::text::AnyString;
using el::text::AnyStringBuilder;
using el::text::String;
using el::text::StringConverter;
using el::text::TextDocument;
using el::text::TextNode;
using el::text::TextNodePtr;
using el::text::U16StringEditor;
using el::text::U32StringEditor;
using el::text::U8StringEditor;
using el::text::html::HtmlParser;

TESTED_TARGETS(HtmlParser TextDocument TextNode)
class HtmlParserTest final : public el::UnitTest {
public:
    void testParseBuildsInlineStructureForFragments() {

        const auto document =
            parse(R"(This is a <strong>text</strong> with <span class="hl"><em>highlights</em></span>.)"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Paragraph",
                "    Text text=\"This is a \"",
                "    Strong",
                "      Text text=\"text\"",
                "    Text text=\" with \"",
                "    Span style=\"hl\"",
                "      Emphasis",
                "        Text text=\"highlights\"",
                "    Text text=\".\"",
            });
    }

    void testParseDropsIgnoredDocumentBoilerplateAndKeepsBodyContent() {

        const auto document = parse(
            "<!doctype html><html lang=en><head><title>Ignored</title><script>hidden()</script></head>"
            "<body><h1>Title</h1><!-- hidden --><p>Visible <b>text</b></p></body></html>"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Heading level=1",
                "    Text text=\"Title\"",
                "  Paragraph",
                "    Text text=\"Visible \"",
                "    Strong",
                "      Text text=\"text\"",
            });
    }

    void testParseRecoversMissingListItemEndTags() {

        const auto document = parse("A test with a list.<ul><li>Entry<li>Another entry</li></ul>"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Paragraph",
                "    Text text=\"A test with a list.\"",
                "  BulletList",
                "    BulletListItem",
                "      Text text=\"Entry\"",
                "    BulletListItem",
                "      Text text=\"Another entry\"",
            });
    }

    void testParseCreatesNumberedListItems() {

        const auto document = parse("<ol><li>One<li>Two</ol>"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  NumberedList",
                "    NumberedListItem",
                "      Text text=\"One\"",
                "    NumberedListItem",
                "      Text text=\"Two\"",
            });
    }

    void testParseClosesNestedChildrenWhenAParentEndTagAppears() {

        const auto document = parse("<p><strong>bold</p>tail"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Paragraph",
                "    Strong",
                "      Text text=\"bold\"",
                "  Paragraph",
                "    Text text=\"tail\"",
            });
    }

    void testParseCreatesUnsupportedPlaceholdersOncePerUnsupportedElement() {

        const auto document = parse(
            R"(<img class="hero"/><table><tr><td>hidden</td></tr></table><form><input>ignored</form><svg><text>ignored</text></svg>)"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Unsupported style=\"hero\" text=\"image\"",
                "  Unsupported text=\"table\"",
                "  Unsupported text=\"form\"",
                "  Unsupported text=\"svg\"",
            });
    }

    void testParsePreservesWhitespaceInsidePreformattedBlocks() {

        const auto document = parse("<pre>  a\n  b</pre>"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  CodeBlock",
                "    Text text=\"  a\\n  b\"",
            });
    }

    void testParseMapsAttributesToCreatedNodes() {

        const auto document =
            parse(R"(<a id="link-id" class="cta" href="/target">Go</a><hr id="rule" class="sep">)"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Paragraph",
                "    Link id=\"link-id\" style=\"cta\" data=\"/target\"",
                "      Text text=\"Go\"",
                "  HorizontalLine id=\"rule\" style=\"sep\"",
            });
        const auto link = document.root()->children().first()->children().first();
        REQUIRE(std::dynamic_pointer_cast<const el::text::impl::LinkData>(link->data()) != nullptr);
    }

    void testParseHandlesSelfClosingInlineTags() {

        const auto document = parse(R"(before<br/>after <span class="x"/>tail)"_el);

        requireTreeEqual(
            document,
            {
                "Document",
                "  Paragraph",
                "    Text text=\"before\"",
                "    LineBreak",
                "    Text text=\"after \"",
                "    Span style=\"x\"",
                "    Text text=\"tail\"",
            });
    }

    void testPublicFacadeAndEncodings() {

        auto parser = HtmlParser{String{"<p>Hello <em>world</em></p>"_el}};
        requirePlainText(parser.parse(), "Hello world");
        requirePlainText(parser.parseOrThrow(), "Hello world");

        auto aliasParser = el::html::HtmlParser{String{"<strong>Alias</strong>"_el}};
        requirePlainText(aliasParser.parse(), "Alias");

        const auto utf8 = U8StringEditor{std::u8string_view{u8"<p>A¢</p>"}};
        const auto utf16 = U16StringEditor{std::u16string_view{u"<p>A¢</p>"}};
        const auto utf32 = U32StringEditor{std::u32string_view{U"<p>A¢</p>"}};
        requirePlainText(HtmlParser{AnyString{utf8}}.parse(), "A¢");
        requirePlainText(HtmlParser{AnyString{utf16}}.parse(), "A¢");
        requirePlainText(HtmlParser{AnyString{utf32}}.parse(), "A¢");
    }

    void testMalformedHtmlIsRecoveredByBothParseMethods() {

        auto parser = HtmlParser{String{R"(prefix <strong title="A &amp; B suffix)"_el}};
        requirePlainText(parser.parse(), R"(prefix <strong title="A & B suffix)");
        requirePlainText(parser.parseOrThrow(), R"(prefix <strong title="A & B suffix)");
    }

private:
    [[nodiscard]] static auto parse(String html) -> TextDocument { return HtmlParser{html}.parse(); }

    void requirePlainText(const TextDocument &document, const std::string &expected) {
        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), expected);
    }

    void requireTreeEqual(const TextDocument &document, const std::initializer_list<std::string_view> expectedLines) {
        auto actual = std::vector<std::string>{};
        appendNode(actual, document.root(), 0);

        REQUIRE_EQUAL(actual.size(), expectedLines.size());
        auto index = std::size_t{0};
        for (const auto expectedLine : expectedLines) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE_EQUAL(actual[index], std::string{expectedLine}); },
                [&]() -> std::string { return "index = " + std::to_string(index); });
            index += 1;
        }
    }

    void appendNode(std::vector<std::string> &lines, const TextNodePtr &node, const int indent) {
        if (!node) {
            return;
        }
        auto line = std::string(static_cast<std::size_t>(indent), ' ');
        line += StringConverter{node->type().toString()}.toStdString();
        if (node->level() != 0) {
            line += " level=" + std::to_string(node->level());
        }
        appendProperty(line, "id", node->identifier());
        appendProperty(line, "style", node->style());
        if (node->data() != nullptr) {
            appendProperty(line, "data", node->data()->toString());
        }
        appendProperty(line, "text", node->text());
        lines.push_back(std::move(line));
        for (const auto &child : node->children()) {
            appendNode(lines, child, indent + 2);
        }
    }

    void appendProperty(std::string &line, const std::string &name, String value) {
        if (value.isEmpty()) {
            return;
        }
        line += " " + name + "=\"";
        line += escape(value);
        line += "\"";
    }

    [[nodiscard]] auto escape(String value) -> std::string {
        auto reader = el::text::StringCharReader{value};
        auto builder = std::string{};
        while (!reader.isAtEnd()) {
            const auto character = reader.read();
            if (character == U'\n') {
                builder += "\\n";
                continue;
            }
            builder += StringConverter{AnyStringBuilder{}.append(character).takeString()}.toStdString();
        }
        return builder;
    }
};
