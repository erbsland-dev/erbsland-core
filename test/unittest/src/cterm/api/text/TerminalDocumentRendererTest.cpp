// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../buffer/CursorWriterTestProbe.hpp"

#include "../../support/BlockStringTestHelper.hpp"

#include <erbsland/cterm/CursorBuffer.hpp>
#include <erbsland/cterm/impl/document_renderer/Helpers.hpp>
#include <erbsland/cterm/impl/document_renderer/InlineTextBuilder.hpp>
#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/cterm/TerminalDocumentStyle.hpp>
#include <erbsland/err/DiagnosticHelper.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathErrorContext.hpp>
#include <erbsland/system/PosixErrorContext.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace text = erbsland::text;
namespace renderer_impl = erbsland::cterm::impl::document_renderer;

TESTED_TARGETS(TerminalDocumentRenderer)
class TerminalDocumentRendererTest final : public UNITTEST_SUBCLASS(BlockStringTestHelper) {
public:
    [[nodiscard]] static auto rawLinesFromBuffer(const CursorBuffer &buffer) -> std::vector<std::string> {
        auto lines = std::vector<std::string>{};
        for (auto y = 0; y < buffer.size().height().toRawValue(); ++y) {
            auto line = std::string{};
            for (auto x = 0; x < buffer.size().width().toRawValue(); ++x) {
                line += blockToStdString(buffer.get(bgeo::BlockPosition{x, y}));
            }
            lines.emplace_back(std::move(line));
        }
        return lines;
    }

    [[nodiscard]] static auto renderDocument(
        const TerminalDocumentRenderer &renderer, const text::TextDocument &document, const int width = 80)
        -> std::string {
        auto buffer = CursorBuffer{
            bgeo::BlockSize{width, 1}, CursorBuffer::OverflowMode::ExpandThenWrap, bgeo::BlockSize{width, 1000}};
        renderer.renderTo(buffer, document);
        auto lines = rawLinesFromBuffer(buffer);
        for (auto &line : lines) {
            while (!line.empty() && line.back() == ' ') {
                line.pop_back();
            }
        }
        while (!lines.empty() && lines.back().empty()) {
            lines.pop_back();
        }
        auto result = std::string{};
        for (auto index = std::size_t{0}; index < lines.size(); ++index) {
            if (index > 0) {
                result += '\n';
            }
            result += lines[index];
        }
        return result;
    }

    void testWriterRenderingUsesInlineStylesAndListMarkers() {
        auto document = text::TextDocument{};
        document.addHeading(1)->addText("Title"_el);
        auto paragraph = document.addParagraph();
        paragraph->addText("  Hello "_el);
        paragraph->addStrong()->addText("world  "_el);

        auto bulletList = document.addBulletList(0);
        bulletList->addListItem()->addParagraph()->addText("first"_el);
        bulletList->addListItem()->addText("second"_el);

        auto numberedList = document.addNumberedList(0);
        numberedList->addListItem()->addText("one"_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        const auto result = renderDocument(renderer, document);

        REQUIRE_EQUAL(
            result,
            std::string{"\nTitle\n"
                        "Hello world\n"
                        "•   first\n"
                        "•   second\n"
                        "1.  one"});
    }

    void testConsecutiveInlineListItemChildrenShareOneParagraph() {
        auto document = text::TextDocument{};
        auto item = document.addNumberedList(0)->addListItem();
        item->addStrong()->addText("White tea"_el);
        item->addText(", made from young buds or lightly handled leaves."_el);

        auto renderer = TerminalDocumentRenderer{};
        REQUIRE_EQUAL(
            renderDocument(renderer, document),
            std::string{"1.  White tea, made from young buds or lightly handled leaves."});
    }

    void testRendererCanRenderRepeatedDocuments() {
        auto firstDocument = text::TextDocument{};
        firstDocument.addParagraph()->addText("first"_el);
        auto secondDocument = text::TextDocument{};
        secondDocument.addParagraph()->addText("second"_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        REQUIRE_EQUAL(renderDocument(renderer, firstDocument), std::string{"first"});

        REQUIRE_EQUAL(renderDocument(renderer, secondDocument), std::string{"second"});
    }

    void testFieldListUsesFormLayout() {
        auto document = text::TextDocument{};
        auto list = document.root()->add(text::TextNodeType::FieldList);
        auto first = list->add(text::TextNodeType::FieldItem);
        first->add(text::TextNodeType::FieldLabel)->addText("Path"_el);
        first->add(text::TextNodeType::FieldContent)->addText("/tmp/report.txt"_el);
        auto second = list->add(text::TextNodeType::FieldItem);
        second->add(text::TextNodeType::FieldLabel)->addText("Code"_el);
        second->add(text::TextNodeType::FieldContent)->addText("ENOENT"_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        const auto result = renderDocument(renderer, document);

        REQUIRE(result.find("Path:") != std::string::npos);
        REQUIRE(result.find("/tmp/report.txt") != std::string::npos);
        REQUIRE(result.find("Code:") != std::string::npos);
    }

    void testFieldListAlignsAtFirstAvailableColumn() {
        auto document = text::TextDocument{};
        auto list = document.root()->add(text::TextNodeType::FieldList);
        auto code = list->add(text::TextNodeType::FieldItem);
        code->add(text::TextNodeType::FieldLabel)->addText("errno"_el);
        code->add(text::TextNodeType::FieldContent)->addText("2"_el);
        auto message = list->add(text::TextNodeType::FieldItem);
        message->add(text::TextNodeType::FieldLabel)->addText("message"_el);
        message->add(text::TextNodeType::FieldContent)->addText("No such file or directory"_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        auto buffer = CursorBuffer{bgeo::BlockSize{80, 2}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);
        const auto lines = rawLinesFromBuffer(buffer);
        REQUIRE(lines[0].find("errno:   2") != std::string::npos);
        REQUIRE(lines[1].find("message: No such file or directory") != std::string::npos);
    }

    void testContainerDecorationsSurroundAndPrefixContent() {
        auto style = TerminalDocumentStyle{};
        style.edit(TerminalDocumentStyleSelector::blockquote())
            .setPrefix("top"_el)
            .setLinePrefix("| "_el)
            .setSuffix("bottom"_el);
        auto document = text::TextDocument{};
        document.addBlockquote()->add(text::TextNodeType::Paragraph)->addText("content"_el);

        auto renderer = TerminalDocumentRenderer{style};
        REQUIRE_EQUAL(renderDocument(renderer, document), std::string{"top\n| content\nbottom"});
    }

    void testRenderToCursorBufferUsesMarginsAndTerminalWidthRules() {
        auto style = TerminalDocumentStyle{};
        style.edit(TerminalDocumentStyleSelector::horizontalLine()).setMargins(bgeo::BlockMargins{1, 2, 0, 3});

        auto document = text::TextDocument{};
        document.addHorizontalLine();

        auto renderer = TerminalDocumentRenderer{style};
        auto writer = CursorWriterProbe{};
        writer._size = bgeo::BlockSize{12, 3};
        renderer.renderTo(writer, document);

        REQUIRE_EQUAL(writer._writtenStrings.size(), std::size_t{2});
        REQUIRE(writer._writtenStrings[0].isEmpty());
        REQUIRE_EQUAL(writer._writtenStrings[1].displayWidth(), 78);
        REQUIRE_EQUAL(render(writer._writtenStrings[1]).substr(0, 6), std::string{"   ---"});
        REQUIRE_EQUAL(writer._lineBreakCount, 2);
    }

    void testDeeplyNestedListsRenderWithoutDroppingItems() {
        auto document = text::TextDocument{};
        auto list = document.addBulletList(0);
        for (auto level = 0; level < 10; ++level) {
            auto item = list->addListItem();
            item->addText(text::StringEditor{std::string{"level "} + std::to_string(level)});
            if (level + 1 < 10) {
                list = item->addBulletList(level + 1);
            }
        }

        auto renderer = TerminalDocumentRenderer{};
        const auto rendered = renderDocument(renderer, document);

        REQUIRE(rendered.find("level 0") != std::string::npos);
        REQUIRE(rendered.find("level 9") != std::string::npos);
        REQUIRE_EQUAL(std::count(rendered.begin(), rendered.end(), '\n'), 9);
    }

    void testManyInlineSpansRenderWithoutTextLoss() {
        auto document = text::TextDocument{};
        auto paragraph = document.addParagraph();
        auto expected = std::string{};
        for (auto index = 0; index < 128; ++index) {
            auto span = paragraph->addSpan();
            span->setStyle(" alpha beta alpha "_el);
            span->addText("x"_el);
            expected += 'x';
        }

        auto renderer = TerminalDocumentRenderer{};
        REQUIRE_EQUAL(renderDocument(renderer, document), expected.substr(0, 79) + "-\n" + expected.substr(79));
    }

    void testSemanticSeparatorsAndEscapeSequencesControlWrappingWithoutHiddenCharacters() {
        const auto prefix = text::StringEditor{std::string(58, 'a')};
        auto separatorDocument = text::TextDocument{};
        auto separatorParagraph = separatorDocument.addParagraph();
        separatorParagraph->addText(prefix);
        separatorParagraph->add(text::TextNodeType::Separator)->addText("/"_el);
        separatorParagraph->addText("tail"_el);

        auto escapeDocument = text::TextDocument{};
        auto escapeParagraph = escapeDocument.addParagraph();
        escapeParagraph->addText(prefix);
        escapeParagraph->add(text::TextNodeType::EscapeSequence)->addText("\\033"_el);
        escapeParagraph->addText("tail"_el);

        auto oversizedEscapeDocument = text::TextDocument{};
        auto oversizedEscapeParagraph = oversizedEscapeDocument.addParagraph();
        oversizedEscapeParagraph->add(text::TextNodeType::EscapeSequence)
            ->addText(text::StringEditor{std::string(70, 'e')});
        oversizedEscapeParagraph->addText("tail"_el);

        auto renderer = TerminalDocumentRenderer{};
        REQUIRE_EQUAL(renderDocument(renderer, separatorDocument, 60), std::string(58, 'a') + "/\ntail");
        REQUIRE_EQUAL(renderDocument(renderer, escapeDocument, 60), std::string(58, 'a') + "\n\\033tail");
        auto oversizedWriter = CursorWriterProbe{};
        oversizedWriter._size = bgeo::BlockSize{60, 25};
        renderer.renderTo(oversizedWriter, oversizedEscapeDocument);
        REQUIRE_EQUAL(oversizedWriter._writtenStrings.size(), std::size_t{2});
        REQUIRE_EQUAL(render(oversizedWriter._writtenStrings[0]), std::string(70, 'e'));
        REQUIRE_EQUAL(render(oversizedWriter._writtenStrings[1]), std::string{"tail"});
        REQUIRE(renderDocument(renderer, separatorDocument, 60).find("\u200b") == std::string::npos);
    }

    void testEffectiveWidthSnapshotsUseOneLayoutAlgorithm() {
        auto document = text::TextDocument{};
        document.addParagraph()->addText(text::StringEditor{std::string(70, 'x')});
        auto renderer = TerminalDocumentRenderer{};

        REQUIRE_EQUAL(renderDocument(renderer, document, 60), std::string(59, 'x') + "-\n" + std::string(11, 'x'));
        REQUIRE_EQUAL(renderDocument(renderer, document, 80), std::string(70, 'x'));
        REQUIRE_EQUAL(renderDocument(renderer, document, 120), std::string(70, 'x'));

        auto narrowWriter = CursorWriterProbe{};
        narrowWriter._size = bgeo::BlockSize{1, 25};
        renderer.renderTo(narrowWriter, document);
        REQUIRE_EQUAL(narrowWriter._writtenStrings.size(), std::size_t{1});
        REQUIRE_EQUAL(render(narrowWriter._writtenStrings[0]), std::string(70, 'x'));
    }

    void testFramedDiagnosticUsesOnePrefixColumnForAllPhysicalLines() {
        const auto platform = std::make_shared<const erbsland::system::PosixErrorContext>(2, "No such file"_el);
        const auto pathError = erbsland::path::PathError{erbsland::path::PathErrorContext{
            "Path could not be resolved"_el, "The path could not be resolved to its physical location."_el}
                .setSourcePath("/tmp/missing"_el)
                .setPlatformContext(platform)};
        const auto error = erbsland::err::Exception{"Root error"_el, std::make_exception_ptr(pathError)};
        const auto document = erbsland::err::DiagnosticHelper{error}.toDocument();

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        const auto rendered = renderDocument(renderer, document);
        const auto expected = std::vector<std::string>{
            "",
            "  Root error",
            "",
            "╭─ Caused By:",
            "│   Path could not be resolved",
            "│   The path could not be resolved to its physical location.",
            "│   Check that the path is correct and that every parent directory exists.",
            "│",
            "│ Paths:",
            "│   path: /tmp/missing",
            "│",
            "│ Platform Error:",
            "│   errno:   2",
            "│   message: No such file",
            "╰─"};
        const auto actual = erbsland::unittest::th::splitLines(rendered);
        REQUIRE_EQUAL_LINES(actual, expected);
    }

    void testTermListRenderingUsesColumnsAndOptionNames() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto first = list->add(text::TextNodeType::TermItem);
        auto firstName = first->add(text::TextNodeType::TermName);
        auto firstShort = firstName->add(text::TextNodeType::OptionName);
        firstShort->add(text::TextNodeType::OptionShort)->addText("-h"_el);
        auto firstLong = firstName->add(text::TextNodeType::OptionName);
        firstLong->add(text::TextNodeType::OptionLong)->addText("--help"_el);
        first->add(text::TextNodeType::TermDescription)->addText("Display help."_el);

        auto second = list->add(text::TextNodeType::TermItem);
        auto secondName = second->add(text::TextNodeType::TermName);
        auto secondLong = secondName->add(text::TextNodeType::OptionName);
        secondLong->add(text::TextNodeType::OptionLong)->addText("--long"_el);
        secondName->addText(" "_el);
        secondName->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("value"_el);
        second->add(text::TextNodeType::TermDescription)->addText("Long only."_el);

        auto third = list->add(text::TextNodeType::TermItem);
        auto thirdName = third->add(text::TextNodeType::TermName);
        auto thirdLong = thirdName->add(text::TextNodeType::OptionName);
        thirdLong->add(text::TextNodeType::OptionLong)->addText("--flag"_el);
        third->add(text::TextNodeType::TermDescription)->addText("Flag only."_el);

        auto fourth = list->add(text::TextNodeType::TermItem);
        auto fourthName = fourth->add(text::TextNodeType::TermName);
        fourthName->add(text::TextNodeType::OptionMeta)->setStyle("positional"_el).addText("input"_el);
        fourth->add(text::TextNodeType::TermDescription)->addText("Positional."_el);

        auto renderer = TerminalDocumentRenderer{};
        const auto rendered = renderDocument(renderer, document);
        REQUIRE_EQUAL(
            rendered,
            std::string{"-h, --help        Display help.\n"
                        "    --long value  Long only.\n"
                        "    --flag        Flag only.\n"
                        "    input         Positional."});
        REQUIRE(rendered.find('\t') == std::string::npos);
    }

    void testShortOptionRowsEnableLongOnlyFlagIndent() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto first = list->add(text::TextNodeType::TermItem);
        auto firstName = first->add(text::TextNodeType::TermName);
        auto firstShort = firstName->add(text::TextNodeType::OptionName);
        firstShort->add(text::TextNodeType::OptionShort)->addText("-h"_el);
        auto firstLong = firstName->add(text::TextNodeType::OptionName);
        firstLong->add(text::TextNodeType::OptionLong)->addText("--help"_el);
        first->add(text::TextNodeType::TermDescription)->addText("Display help."_el);

        auto second = list->add(text::TextNodeType::TermItem);
        auto secondName = second->add(text::TextNodeType::TermName);
        auto secondLong = secondName->add(text::TextNodeType::OptionName);
        secondLong->add(text::TextNodeType::OptionLong)->addText("--version"_el);
        second->add(text::TextNodeType::TermDescription)->addText("Display version."_el);

        auto renderer = TerminalDocumentRenderer{};
        REQUIRE_EQUAL(
            renderDocument(renderer, document),
            std::string{"-h, --help     Display help.\n"
                        "    --version  Display version."});
    }

    void testOptionTermListKeepsMetaValuesInFirstColumn() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        auto name = item->add(text::TextNodeType::TermName);
        auto optionName = name->add(text::TextNodeType::OptionName);
        optionName->add(text::TextNodeType::OptionShort)->addText("-c"_el);
        auto longName = name->add(text::TextNodeType::OptionName);
        longName->add(text::TextNodeType::OptionLong)->addText("--count"_el);
        name->addText(" "_el);
        name->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("integer"_el);
        item->add(text::TextNodeType::TermDescription)->addText("Number of readings."_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        REQUIRE_EQUAL(renderDocument(renderer, document), std::string{"  -c, --count <integer>  Number of readings."});
    }

    void testCodeSnippetUsesStableGutterAndMarkerStyles() {
        auto document = text::TextDocument{};
        auto lines = text::StringList{};
        lines.append("  padded value"_el);
        lines.append("next"_el);
        auto markers = text::CodeSnippetMarkerList{};
        markers.append(
            text::CodeSnippetMarker{
                el::unit::LineIndex{12U}, el::unit::ColumnIndex{2U}, el::unit::ColumnCount{6U}, {}, "error"_el});
        document.addCodeSnippet(
            el::text::CodeSnippet{std::move(lines), el::unit::LineIndex{12U}, "demo"_el}, std::move(markers));

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        const auto rendered = renderDocument(renderer, document);
        REQUIRE(rendered.find("  13 \u2502   padded value") != std::string::npos);
        REQUIRE(rendered.find("     \u2502   \u2594\u2594\u2594\u2594\u2594\u2594") != std::string::npos);
        REQUIRE(rendered.find("  14 \u2502 next") != std::string::npos);

        auto buffer = CursorBuffer{bgeo::BlockSize{40, 6}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);
        const auto rawLines = rawLinesFromBuffer(buffer);
        auto markerY = std::size_t{0U};
        while (markerY < rawLines.size() && rawLines[markerY].find("\u2594") == std::string::npos) {
            ++markerY;
        }
        REQUIRE(markerY < rawLines.size());
        const auto gutterX = rawLines[markerY].find("\u2502");
        const auto markerX = rawLines[markerY].find("\u2594");
        REQUIRE(gutterX != std::string::npos);
        REQUIRE(markerX != std::string::npos);
        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{static_cast<int>(gutterX), static_cast<int>(markerY)}).style().fg(),
            fg::BrightBlack);
        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{static_cast<int>(markerX), static_cast<int>(markerY)}).style().fg(),
            fg::BrightRed);
    }

    void testCodeSnippetWrapsPointMarkersAndWideCharacters() {
        auto document = text::TextDocument{};
        auto lines = text::StringList{};
        lines.append("abcdefghijklmnop"_el);
        lines.append(text::StringEditor{std::string_view{el::unittest::th::stdStringFromHex("41 E7 95 8C 42")}});
        auto markers = text::CodeSnippetMarkerList{};
        markers.append(
            text::CodeSnippetMarker{el::unit::LineIndex{0U}, el::unit::ColumnIndex{6U}, {}, "here"_el, "error"_el});
        markers.append(
            text::CodeSnippetMarker{
                el::unit::LineIndex{1U}, el::unit::ColumnIndex{1U}, el::unit::ColumnCount{1U}, {}, "warning"_el});
        document.addCodeSnippet(
            el::text::CodeSnippet{std::move(lines), el::unit::LineIndex::zero(), {}}, std::move(markers));

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        const auto rendered = renderDocument(renderer, document);
        REQUIRE(rendered.find("A\u754cB") != std::string::npos);
        REQUIRE(rendered.find("\u2594\u2594") != std::string::npos);

        auto buffer = CursorBuffer{bgeo::BlockSize{16, 12}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);
        const auto rawLines = rawLinesFromBuffer(buffer);
        REQUIRE(
            std::ranges::any_of(rawLines, [](const auto &line) { return line.find("\u2191") != std::string::npos; }));
        REQUIRE(
            std::ranges::any_of(rawLines, [](const auto &line) { return line.find("\u2502") != std::string::npos; }));
    }

    void testWideTermListAlignsDescriptionsInCursorOutput() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);

        auto first = list->add(text::TextNodeType::TermItem);
        auto firstName = first->add(text::TextNodeType::TermName);
        auto firstLong = firstName->add(text::TextNodeType::OptionName);
        firstLong->add(text::TextNodeType::OptionLong)->addText("--config"_el);
        firstName->addText(" "_el);
        firstName->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("value"_el);
        first->add(text::TextNodeType::TermDescription)->addText("Configuration file."_el);

        auto second = list->add(text::TextNodeType::TermItem);
        auto secondName = second->add(text::TextNodeType::TermName);
        auto secondLong = secondName->add(text::TextNodeType::OptionName);
        secondLong->add(text::TextNodeType::OptionLong)->addText("--workspace"_el);
        secondName->addText(" "_el);
        secondName->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("value"_el);
        second->add(text::TextNodeType::TermDescription)->addText("Workspace directory."_el);

        auto third = list->add(text::TextNodeType::TermItem);
        auto thirdName = third->add(text::TextNodeType::TermName);
        auto thirdShort = thirdName->add(text::TextNodeType::OptionName);
        thirdShort->add(text::TextNodeType::OptionShort)->addText("-v"_el);
        auto thirdLong = thirdName->add(text::TextNodeType::OptionName);
        thirdLong->add(text::TextNodeType::OptionLong)->addText("--verbose"_el);
        third->add(text::TextNodeType::TermDescription)->addText("Increase diagnostics."_el);

        auto fourth = list->add(text::TextNodeType::TermItem);
        auto fourthName = fourth->add(text::TextNodeType::TermName);
        auto fourthLong = fourthName->add(text::TextNodeType::OptionName);
        fourthLong->add(text::TextNodeType::OptionLong)->addText("--dry-run"_el);
        fourth->add(text::TextNodeType::TermDescription)->addText("Preview changes."_el);

        auto fifth = list->add(text::TextNodeType::TermItem);
        auto fifthName = fifth->add(text::TextNodeType::TermName);
        fifthName->add(text::TextNodeType::OptionMeta)->setStyle("positional"_el).addText("target"_el);
        fifth->add(text::TextNodeType::TermDescription)->addText("Target name."_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        auto buffer = CursorBuffer{bgeo::BlockSize{80, 6}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);

        const auto lines = rawLinesFromBuffer(buffer);
        REQUIRE(lines[0].starts_with("      --config <value>     Configuration"));
        REQUIRE(lines[1].starts_with("      --workspace <value>  Workspace"));
        REQUIRE(lines[2].starts_with("  -v, --verbose            Increase"));
        REQUIRE(lines[3].starts_with("      --dry-run            Preview"));
        REQUIRE(lines[4].starts_with("      <target>             Target"));
        const auto descriptionColumn = lines[0].find("Configuration");
        REQUIRE(descriptionColumn != std::string::npos);
        REQUIRE_EQUAL(descriptionColumn, std::size_t{27});
        REQUIRE_EQUAL(descriptionColumn, lines[1].find("Workspace"));
        REQUIRE_EQUAL(descriptionColumn, lines[2].find("Increase"));
        REQUIRE_EQUAL(descriptionColumn, lines[3].find("Preview"));
        REQUIRE_EQUAL(descriptionColumn, lines[4].find("Target"));
    }

    void testNestedOptionDetailsUseAdditionalIndent() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        auto name = item->add(text::TextNodeType::TermName);
        auto optionName = name->add(text::TextNodeType::OptionName);
        optionName->add(text::TextNodeType::OptionLong)->addText("--format"_el);
        name->addText(" "_el);
        name->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("choice"_el);
        item->add(text::TextNodeType::TermDescription)->addText("Format of the report."_el);

        auto details = item->add(text::TextNodeType::TermList);
        details->setStyle("option-details"_el);
        auto detail = details->add(text::TextNodeType::TermItem);
        detail->add(text::TextNodeType::TermName)->add(text::TextNodeType::OptionModule)->addText("json"_el);
        detail->add(text::TextNodeType::TermDescription)->addText("Structured data."_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        auto buffer = CursorBuffer{bgeo::BlockSize{100, 3}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);

        const auto lines = rawLinesFromBuffer(buffer);
        REQUIRE(lines[0].starts_with("      --format <choice>  Format"));
        REQUIRE(lines[1].starts_with("      json  Structured"));
    }

    void testAlignedTermListUsesQuarterWidthDescriptionColumn() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        item->add(text::TextNodeType::TermName)->addText("parameter"_el);
        item->add(text::TextNodeType::TermDescription)
            ->addText(
                "Description text that is intentionally long enough to force aligned layout instead of the perfect "
                "layout mode."_el);

        auto renderer = TerminalDocumentRenderer{};
        auto buffer = CursorBuffer{bgeo::BlockSize{100, 4}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);

        const auto lines = rawLinesFromBuffer(buffer);
        REQUIRE_EQUAL(lines[0].find("Description"), std::size_t{25});
    }

    void testLongTermNameBreaksDescriptionAtTabStop() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        item->add(text::TextNodeType::TermName)->addText("very-long-option-name-that-overflows"_el);
        item->add(text::TextNodeType::TermDescription)
            ->addText("Description text that is long enough to select aligned layout."_el);

        auto renderer = TerminalDocumentRenderer{};
        auto buffer = CursorBuffer{bgeo::BlockSize{80, 4}, CursorBuffer::OverflowMode::Wrap};
        renderer.renderTo(buffer, document);

        const auto lines = rawLinesFromBuffer(buffer);
        REQUIRE(lines[0].find("very-long-option-name-that-overflows") != std::string::npos);
        REQUIRE_EQUAL(lines[1].find("Description"), std::size_t{20});
    }

    void testUnknownWidthUsesNormalEightyColumnRendering() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        item->add(text::TextNodeType::TermName)->addText("name"_el);
        item->add(text::TextNodeType::TermDescription)->addText("description"_el);

        auto renderer = TerminalDocumentRenderer{};
        auto writer = CursorWriterProbe{};
        writer._size = bgeo::BlockSize{0, 25};
        renderer.renderTo(writer, document);

        REQUIRE_EQUAL(writer._writtenStrings.size(), std::size_t{1});
        REQUIRE_EQUAL(render(writer._writtenStrings[0]), std::string{"name  description"});
        REQUIRE_EQUAL(writer._lineBreakCount, 1);
        REQUIRE(writer._lastParagraph.isEmpty());
    }

    void testSystemOutputStyleAddsOptionSemanticSuffixes() {
        auto document = text::TextDocument{};
        auto heading = document.addHeading(2);
        heading->setStyle("option-section"_el);
        heading->addText("Usage"_el);

        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        auto name = item->add(text::TextNodeType::TermName);
        name->setStyle("option-label"_el);
        name->addText("Author"_el);
        item->add(text::TextNodeType::TermDescription)->addText("Tobias Erbsland"_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};

        REQUIRE_EQUAL(renderDocument(renderer, document), std::string{"\nUsage:\n  Author:  Tobias Erbsland"});
    }

    void testNarrowWriterReceivesEightyColumnLayoutWithStyles() {
        auto document = text::TextDocument{};
        auto list = document.add(text::TextNodeType::TermList);
        auto item = list->add(text::TextNodeType::TermItem);
        auto name = item->add(text::TextNodeType::TermName);
        auto optionName = name->add(text::TextNodeType::OptionName);
        optionName->add(text::TextNodeType::OptionLong)->addText("--config"_el);
        name->addText(" "_el);
        name->add(text::TextNodeType::OptionMeta)->setStyle("value"_el).addText("path"_el);
        item->add(text::TextNodeType::TermDescription)->addText("Use path."_el);

        auto renderer = TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()};
        auto writer = CursorWriterProbe{};
        writer._size = bgeo::BlockSize{40, 4};
        renderer.renderTo(writer, document);

        REQUIRE_EQUAL(writer._writtenStrings.size(), std::size_t{2});
        const auto &line = writer._writtenStrings[0];
        REQUIRE(writer._writtenStrings[1].isEmpty());
        REQUIRE(render(line).starts_with("      --config <path>  Use path."));
        REQUIRE_EQUAL(line[BlockIndex{6U}].style().fg(), fg::BrightCyan);
        REQUIRE_EQUAL(line[BlockIndex{15U}].style().fg(), fg::BrightBlack);
        REQUIRE_EQUAL(line[BlockIndex{16U}].style().fg(), fg::BrightGreen);
        REQUIRE_EQUAL(line[BlockIndex{20U}].style().fg(), fg::BrightBlack);
        REQUIRE_EQUAL(line[BlockIndex{6U}].style().bg(), bg::Default);
    }

    void testLargeDocumentRepeatedRendersStayStable() {
        auto document = text::TextDocument{};
        auto expected = std::string{};
        for (auto index = 0; index < 96; ++index) {
            const auto line = std::string{"paragraph "} + std::to_string(index);
            document.addParagraph()->addText(text::StringEditor{line});
            if (!expected.empty()) {
                expected += '\n';
            }
            expected += line;
        }

        auto renderer = TerminalDocumentRenderer{};
        for (auto pass = 0; pass < 3; ++pass) {
            REQUIRE_EQUAL(renderDocument(renderer, document), expected);
        }
    }

    void testRendererHelpersAreDirectlyTestable() {
        REQUIRE_EQUAL(
            renderer_impl::collapsedVerticalMarginValue(bgeo::BlockCoordinate{2}, bgeo::BlockCoordinate{4}),
            bgeo::BlockCoordinate{4});
        REQUIRE_EQUAL(
            renderer_impl::collapsedVerticalMarginValue(bgeo::BlockCoordinate{-3}, bgeo::BlockCoordinate{-1}),
            bgeo::BlockCoordinate{-3});
        REQUIRE_EQUAL(
            renderer_impl::collapsedVerticalMarginValue(bgeo::BlockCoordinate{4}, bgeo::BlockCoordinate{-1}),
            bgeo::BlockCoordinate{3});

        const auto preservedText = text::StringEditor{std::string{"  alpha  "}};
        auto builder = renderer_impl::InlineTextBuilder{};
        builder.appendText(preservedText, BlockStyle{}, true);
        REQUIRE_EQUAL(render(builder.takeString()), std::string{"  alpha  "});

        const auto decoration = BlockStringEditor{text::U32StringEditor{U"  beta  "}};
        builder.reset();
        builder.appendDecoration(decoration, BlockStyle{}, false);
        REQUIRE_EQUAL(render(builder.takeString()), std::string{"beta"});
    }
};
