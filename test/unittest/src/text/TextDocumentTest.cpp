// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/impl/CodeBlockData.hpp>
#include <erbsland/text/impl/CodeSnippetData.hpp>
#include <erbsland/text/impl/LinkData.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/PlainTextRenderer.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/text/TextNode.hpp>
#include <erbsland/text/TextNodeData.hpp>
#include <erbsland/text/TextNodeType.hpp>
#include <erbsland/text/TextWalkResult.hpp>
#include <erbsland/text/TextWalkStatus.hpp>
#include <erbsland/unit/ColumnCount.hpp>
#include <erbsland/unit/ColumnIndex.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unit/LineIndex.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace {

class TestNodeData final : public el::text::TextNodeData {
public:
    explicit TestNodeData(el::text::String text) noexcept : _text{std::move(text)} {}

public:
    [[nodiscard]] auto toString() const -> el::text::String override { return _text; }

private:
    el::text::String _text;
};

}

using el::text::AnyStringBuilder;
using el::text::PlainTextRenderer;
using el::text::StringConverter;
using el::text::TextDocument;
using el::text::TextNode;
using el::text::TextNodeType;
using el::text::TextWalkResult;
using el::text::TextWalkStatus;
using el::unit::ElementIndex;

TESTED_TARGETS(PlainTextRenderer TextDocument TextNode TextNodeType TextWalkResult TextWalkStatus)
class TextDocumentTest final : public el::UnitTest {
public:
    void testDocumentConstruction() {
        auto document = TextDocument{};

        REQUIRE(document.root() != nullptr);
        REQUIRE_EQUAL(document.root()->type(), TextNodeType::Document);
        REQUIRE(!document.root()->hasParent());
        REQUIRE(document.isEmpty());
        REQUIRE(!std::is_copy_constructible_v<TextDocument>);
        REQUIRE(std::is_move_constructible_v<TextDocument>);
        REQUIRE(!std::is_default_constructible_v<TextNode>);
        REQUIRE(!std::is_constructible_v<TextNode, TextNodeType>);
        REQUIRE(!std::is_copy_constructible_v<PlainTextRenderer>);
        REQUIRE(std::is_move_constructible_v<PlainTextRenderer>);
    }

    void testNodeFactoriesAndMetadata() {
        using namespace el::text::literals;

        auto heading = TextNode::createHeading(2);
        heading->setIdentifier("intro"_el).setStyle("lead"_el).setData(std::make_shared<TestNodeData>("chapter"_el));
        heading->addText("Introduction"_el);

        REQUIRE_EQUAL(heading->type(), TextNodeType::Heading);
        REQUIRE_EQUAL(heading->level(), 2);
        REQUIRE_EQUAL(heading->identifier(), "intro"_el);
        REQUIRE_EQUAL(heading->style(), "lead"_el);
        REQUIRE(heading->data() != nullptr);
        REQUIRE_EQUAL(heading->data()->toString(), "chapter"_el);
        heading->setData(std::make_shared<TestNodeData>("custom"_el));
        REQUIRE_EQUAL(heading->data()->toString(), "custom"_el);
        REQUIRE(heading->hasChildren());
        REQUIRE_EQUAL(heading->children().count().toSizeT(), std::size_t{1U});
        REQUIRE_EQUAL(heading->children().get(ElementIndex::zero())->type(), TextNodeType::Text);
        REQUIRE_EQUAL(heading->children().get(ElementIndex::zero())->text(), "Introduction"_el);
        REQUIRE(heading->children().get(ElementIndex::zero())->hasParent());
        REQUIRE_EQUAL(heading->children().get(ElementIndex::zero())->parent(), heading);

        const auto codeBlock = TextNode::createCodeBlock("cpp"_el);
        REQUIRE(codeBlock->data() != nullptr);
        REQUIRE(std::dynamic_pointer_cast<const el::text::impl::CodeBlockData>(codeBlock->data()) != nullptr);
        REQUIRE_EQUAL(codeBlock->data()->toString(), "cpp"_el);
        REQUIRE(TextNode::createCodeBlock()->data() == nullptr);

        const auto link = TextNode::createLink("https://erbsland.dev"_el);
        REQUIRE(link->data() != nullptr);
        REQUIRE(std::dynamic_pointer_cast<const el::text::impl::LinkData>(link->data()) != nullptr);
        REQUIRE_EQUAL(link->data()->toString(), "https://erbsland.dev"_el);
        REQUIRE(TextNode::createLink()->data() == nullptr);
    }

    void testTraversal() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        document.addHeading(1)->addText("Title"_el);
        document.addParagraph()->addStrong()->addText("bold"_el);

        auto visited = std::size_t{0U};
        const auto result = document.root()->walk([&visited](const TextNode &node) {
            ++visited;
            return node.type() == TextNodeType::Strong ? TextWalkStatus::Stop : TextWalkStatus::Continue;
        });

        REQUIRE_EQUAL(result, TextWalkResult::Stopped);
        REQUIRE_EQUAL(visited, std::size_t{5U});
        REQUIRE(document.root()->anyOf([](const TextNode &node) { return node.type() == TextNodeType::Strong; }));
        REQUIRE(document.root()->contains(TextNodeType::Strong));
        REQUIRE(!document.root()->anyOf([](const TextNode &node) { return node.type() == TextNodeType::Error; }));
        REQUIRE(!document.root()->contains(TextNodeType::Error));
        REQUIRE_EQUAL(
            document.root()->walk([](const TextNode &node) {
                return node.type() == TextNodeType::Paragraph ? TextWalkStatus::Failure : TextWalkStatus::Continue;
            }),
            TextWalkResult::Failure);
    }

    void testClone() {
        using namespace el::text::literals;

        auto heading = TextNode::createHeading(2);
        heading->setIdentifier("intro"_el).setStyle("lead"_el).setData(std::make_shared<TestNodeData>("chapter"_el));
        heading->addStrong()->addText("Hello"_el);

        auto clone = heading->clone();

        REQUIRE(clone != heading);
        REQUIRE_EQUAL(clone->type(), TextNodeType::Heading);
        REQUIRE_EQUAL(clone->level(), 2);
        REQUIRE_EQUAL(clone->identifier(), "intro"_el);
        REQUIRE_EQUAL(clone->style(), "lead"_el);
        REQUIRE(clone->data() != nullptr);
        REQUIRE_EQUAL(clone->data()->toString(), "chapter"_el);
        REQUIRE(!clone->hasParent());
        REQUIRE(clone->contains(TextNodeType::Strong));
        REQUIRE_EQUAL(clone->children().first()->parent(), clone);
        REQUIRE_EQUAL(clone->children().first()->children().first()->text(), "Hello"_el);
    }

    void testNullChildIsRejected() {
        auto paragraph = TextNode::createParagraph();

        REQUIRE_THROWS_AS(el::err::ParameterError, paragraph->add(el::text::TextNodePtr{}));
        REQUIRE(!paragraph->hasChildren());
    }

    void testEscapedTextCreatesSemanticNodesAndToleratesMalformedUtf8() {
        using namespace el::text::literals;

        auto paragraph = TextNode::createParagraph();
        const auto unsafe =
            el::text::StringEditor{std::string_view{el::unittest::th::stdStringFromHex("61 22 5C 1B 62 0A 63 FF")}};
        REQUIRE_EQUAL(&paragraph->addEscapedText(unsafe, el::text::EscapeFormat::Display), paragraph.get());
        REQUIRE_EQUAL(paragraph->children().count(), el::text::TextNodeList::Count{5U});
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{0U}]->type(), TextNodeType::Text);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{0U}]->text(), "a\"\\"_el);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{1U}]->type(), TextNodeType::EscapeSequence);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{1U}]->text(), "\\033"_el);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{2U}]->text(), "b"_el);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{3U}]->type(), TextNodeType::EscapeSequence);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{3U}]->text(), "\\n"_el);
        REQUIRE_EQUAL(paragraph->children()[ElementIndex{4U}]->text(), "c�"_el);

        const auto childCount = paragraph->children().count();
        paragraph->addEscapedText({}, el::text::EscapeFormat::Display);
        REQUIRE_EQUAL(paragraph->children().count(), childCount);
    }

    void testNodeTypeNamesAndClasses() {
        using namespace el::text::literals;

        REQUIRE_EQUAL(TextNodeType{TextNodeType::Document}.toString(), "Document"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::Paragraph}.renderClass(), TextNodeType::RenderClass::Block);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::Emphasis}.renderClass(), TextNodeType::RenderClass::Inline);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::LineBreak}.renderClass(), TextNodeType::RenderClass::Empty);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::BulletListItem}.renderClass(), TextNodeType::RenderClass::Block);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::TermList}.toString(), "TermList"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::OptionLong}.toString(), "OptionLong"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::OptionDetails}.toString(), "OptionDetails"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::CodeSnippet}.toString(), "CodeSnippet"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::CodeLineMarker}.toString(), "CodeLineMarker"_el);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::TermList}.renderClass(), TextNodeType::RenderClass::Structure);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::TermItem}.renderClass(), TextNodeType::RenderClass::Block);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::OptionLong}.renderClass(), TextNodeType::RenderClass::Inline);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::CodeLineMarker}.renderClass(), TextNodeType::RenderClass::Inline);
        REQUIRE_EQUAL(TextNodeType{TextNodeType::CodeSnippet}.renderClass(), TextNodeType::RenderClass::Structure);
        REQUIRE(TextNodeType{TextNodeType::Strong}.isInline());
        REQUIRE(TextNodeType{TextNodeType::OptionMeta}.isInline());
        REQUIRE(TextNodeType{TextNodeType::Paragraph}.isTextContainer());
        REQUIRE(TextNodeType{TextNodeType::BulletListItem}.isTextContainer());
        REQUIRE(TextNodeType{TextNodeType::TermDescription}.isTextContainer());
        REQUIRE(TextNodeType{TextNodeType::CodeLine}.isTextContainer());
        REQUIRE(TextNodeType{TextNodeType::BulletList}.isListContainer());
        REQUIRE(TextNodeType{TextNodeType::TermList}.isListContainer());
        REQUIRE(TextNodeType{TextNodeType::BulletListItem}.isListItem());
        REQUIRE(TextNodeType{TextNodeType::NumberedListItem}.isListItem());
        REQUIRE(TextNodeType{TextNodeType::TermName}.isTermListElement());
        REQUIRE(TextNodeType{TextNodeType::CodeBlock}.preserveWhitespace());
        REQUIRE(TextNodeType{TextNodeType::CodeLine}.preserveWhitespace());
        REQUIRE(!TextNodeType{TextNodeType::CodeLineMarker}.preserveWhitespace());
        REQUIRE(!TextNodeType{TextNodeType::Paragraph}.preserveWhitespace());
        REQUIRE(!TextNodeType{TextNodeType::None}.isTextContainer());

        auto semanticNode = TextNode::create(TextNodeType::OptionDetails);
        REQUIRE_EQUAL(semanticNode->type(), TextNodeType::OptionDetails);
    }

    void testListItemBuilderChoosesListKind() {
        auto paragraph = TextNode::createParagraph();
        REQUIRE(paragraph->addListItem() == nullptr);
        REQUIRE(!paragraph->hasChildren());

        auto bulletList = TextNode::createBulletList(0);
        auto bulletItem = bulletList->addListItem();
        REQUIRE(bulletItem != nullptr);
        REQUIRE_EQUAL(bulletItem->type(), TextNodeType::BulletListItem);

        auto numberedList = TextNode::createNumberedList(0);
        auto numberedItem = numberedList->addListItem();
        REQUIRE(numberedItem != nullptr);
        REQUIRE_EQUAL(numberedItem->type(), TextNodeType::NumberedListItem);
    }

    void testDiagnosticTree() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        auto heading = document.addHeading(1);
        heading->setIdentifier("main"_el).setStyle("title"_el).addText("Hello"_el);

        REQUIRE_EQUAL(
            StringConverter{document.root()->toDiagnosticTree().toString()}.toStdString(),
            std::string{"Document:\n"
                        "    children:\n"
                        "        [0]:\n"
                        "            Heading:\n"
                        "                level: 1\n"
                        "                identifier: main\n"
                        "                style: title\n"
                        "                children:\n"
                        "                    [0]:\n"
                        "                        Text:\n"
                        "                            text: Hello"});
    }

    void testPlainRendering() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        document.addHeading(1)->addText("Overview"_el);
        auto paragraph = document.addParagraph();
        paragraph->addText("Hello "_el);
        paragraph->addStrong()->addText("world"_el);
        paragraph->addLineBreak();
        paragraph->addCode()->addText("x"_el);
        auto bulletList = document.addBulletList();
        bulletList->addListItem()->addText("first"_el);
        auto nestedItem = bulletList->addListItem();
        nestedItem->addText("second"_el);
        nestedItem->addBulletList()->addListItem()->addText("nested"_el);
        auto numberedList = document.addNumberedList();
        numberedList->addListItem()->addText("one"_el);
        numberedList->addListItem()->addText("two"_el);
        document.addCodeBlock("cpp"_el)->addText("return 0;"_el);
        document.addUnsupported("mystery"_el);
        document.addError("broken"_el);

        const auto expected = std::string{"Overview\n"
                                          "Hello world\n"
                                          "x\n"
                                          "- first\n"
                                          "- second\n"
                                          "  - nested\n"
                                          "1. one\n"
                                          "2. two\n"
                                          "return 0;\n"
                                          "mystery\n"
                                          "broken"};
        auto renderer = PlainTextRenderer{document};
        auto builder = AnyStringBuilder{};

        REQUIRE_EQUAL(StringConverter{renderer.build()}.toStdString(), expected);
        REQUIRE_EQUAL(StringConverter{renderer.build()}.toStdString(), expected);
        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), expected);
        REQUIRE_EQUAL(&renderer.appendTo(builder), &builder);
        REQUIRE_EQUAL(StringConverter{builder.toString()}.toStdString(), expected);
    }

    void testPlainTermListRendering() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        auto list = document.add(TextNodeType::TermList);
        auto first = list->add(TextNodeType::TermItem);
        auto firstName = first->add(TextNodeType::TermName);
        auto firstOption = firstName->add(TextNodeType::OptionName);
        firstOption->add(TextNodeType::OptionShort)->addText("-h"_el);
        auto firstLong = firstName->add(TextNodeType::OptionName);
        firstLong->add(TextNodeType::OptionLong)->addText("--help"_el);
        first->add(TextNodeType::TermDescription)->addText("Display help."_el);

        auto second = list->add(TextNodeType::TermItem);
        auto secondName = second->add(TextNodeType::TermName);
        auto secondLong = secondName->add(TextNodeType::OptionName);
        secondLong->add(TextNodeType::OptionLong)->addText("--output"_el);
        secondName->addText(" "_el);
        secondName->add(TextNodeType::OptionMeta)->addText("path"_el);
        auto secondDescription = second->add(TextNodeType::TermDescription);
        secondDescription->addText("Write the result."_el);
        secondDescription->add(TextNodeType::OptionDetails)->addText("Default: stdout."_el);

        auto nested = second->add(TextNodeType::TermList);
        auto nestedItem = nested->add(TextNodeType::TermItem);
        nestedItem->add(TextNodeType::TermName)->addText("json"_el);
        nestedItem->add(TextNodeType::TermDescription)->addText("Structured output."_el);

        const auto expected = std::string{"-h, --help           Display help.\n"
                                          "    --output <path>  Write the result. Default: stdout.\n"
                                          "  json        Structured output."};

        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), expected);
    }

    void testPlainCodeSnippetRendering() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        auto lines = el::text::StringList{};
        lines.append("alpha"_el);
        lines.append("--bad"_el);
        lines.append("omega"_el);
        auto markers = el::text::CodeSnippetMarkerList{};
        markers.append(
            el::text::CodeSnippetMarker{
                el::unit::LineIndex{8U}, el::unit::ColumnIndex::zero(), el::unit::ColumnCount{5U}, {}, "error"_el});
        auto snippet = document.addCodeSnippet(std::move(lines), el::unit::LineIndex{7U}, std::move(markers), "cli"_el);

        REQUIRE_EQUAL(snippet->type(), TextNodeType::CodeSnippet);
        REQUIRE(snippet->data() != nullptr);
        REQUIRE(std::dynamic_pointer_cast<const el::text::impl::CodeSnippetData>(snippet->data()) != nullptr);
        REQUIRE_EQUAL(snippet->data()->toString(), "cli"_el);
        REQUIRE_EQUAL(snippet->children().count().toSizeT(), std::size_t{3U});
        const auto markedLine = snippet->children().get(ElementIndex::one());
        REQUIRE_EQUAL(markedLine->type(), TextNodeType::CodeLine);
        REQUIRE_EQUAL(markedLine->children().count().toSizeT(), std::size_t{3U});
        REQUIRE_EQUAL(markedLine->children().get(ElementIndex::zero())->type(), TextNodeType::CodeLineNumber);
        REQUIRE_EQUAL(markedLine->children().get(ElementIndex::one())->type(), TextNodeType::CodeLineText);
        const auto marker = markedLine->children().get(ElementIndex{2U});
        REQUIRE_EQUAL(marker->type(), TextNodeType::CodeLineMarker);
        REQUIRE(marker->data() != nullptr);
        REQUIRE_EQUAL(marker->data()->toString(), "0:5"_el);
        REQUIRE_EQUAL(marker->style(), "error"_el);

        const auto expected = std::string{"   7 │ alpha\n"
                                          "   8 │ --bad\n"
                                          "     │ \u2594\u2594\u2594\u2594\u2594\n"
                                          "   9 │ omega"};

        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), expected);
    }

    void testPlainCodeSnippetWithoutLineNumbers() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        auto lines = el::text::StringList{};
        lines.append("return 0;"_el);
        document.addCodeSnippet(std::move(lines), el::unit::LineIndex::noIndex());

        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), std::string{"return 0;"});
    }

    void testPlainCodeSnippetUsesFixedWidthAndSafeControls() {
        using namespace el::text::literals;

        auto document = TextDocument{};
        auto lines = el::text::StringList{};
        lines.append(el::text::StringEditor{std::string(100, 'x')});
        lines.append("a\tb"_el);
        document.addCodeSnippet(std::move(lines));

        const auto rendered = StringConverter{document.toString()}.toStdString();
        REQUIRE(rendered.starts_with("   0 │ "));
        REQUIRE(rendered.find("\u2026") != std::string::npos);
        REQUIRE(rendered.find("   1 │ a?b") != std::string::npos);
    }

    void testEmptyRendering() {
        const auto document = TextDocument{};

        REQUIRE_EQUAL(StringConverter{document.toString()}.toStdString(), std::string{});
    }
};
