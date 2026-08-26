// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlainTextRenderer.hpp"

#include "CodeLineMarkerData.hpp"
#include "CodeSnippetLayout.hpp"

#include "../AnyString.hpp"
#include "../AsciiCategory.hpp"
#include "../Literals.hpp"
#include "../StringCharReader.hpp"
#include "../TextDocument.hpp"
#include "../TextNode.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace erbsland::text::impl {

using namespace literals;
using unit::CpLength;

PlainTextRenderer::PlainTextRenderer(const TextDocument &document) noexcept : _document{document} {
}

auto PlainTextRenderer::build() -> String {
    auto builder = AnyStringBuilder{};
    appendTo(builder);
    return builder.toString();
}

auto PlainTextRenderer::appendTo(AnyStringBuilder &builder) -> AnyStringBuilder & {
    resetRenderState();
    renderNode(builder, _document.root());
    return builder;
}

void PlainTextRenderer::resetRenderState() noexcept {
    _firstLine = true;
    _diagnosticDocument =
        std::ranges::any_of(_document.root()->children(), [](const TextNodePtr &node) noexcept -> bool {
            return node != nullptr && node->type() == TextNodeType::Heading && node->level() == 1 &&
                node->style().contains("diagnostic-title"_el) && node->style().contains("error"_el);
        });
    _indent = {};
}

void PlainTextRenderer::renderNode(AnyStringBuilder &builder, const TextNodePtr &node) {
    if (node == nullptr) {
        return;
    }
    switch (node->type().raw()) {
    case TextNodeType::Document:
    case TextNodeType::Section:
    case TextNodeType::DefinitionList:
        renderChildren(builder, *node);
        return;
    case TextNodeType::Blockquote: {
        const auto previousIndent = _indent;
        _indent += CpLength{2U};
        renderChildren(builder, *node);
        _indent = previousIndent;
        return;
    }
    case TextNodeType::CodeSnippet:
        renderCodeSnippet(builder, *node);
        return;
    case TextNodeType::TermList:
    case TextNodeType::FieldList:
        renderTermList(builder, *node);
        return;
    case TextNodeType::BulletList:
        renderList(builder, *node, false);
        return;
    case TextNodeType::NumberedList:
        renderList(builder, *node, true);
        return;
    case TextNodeType::BulletListItem:
    case TextNodeType::NumberedListItem:
        renderListItem(builder, *node, {});
        return;
    case TextNodeType::TermItem:
    case TextNodeType::FieldItem:
        renderTermItem(builder, *node, termListDescriptionColumn(*node));
        return;
    case TextNodeType::LineBreak:
        appendLine(builder, {}, {});
        return;
    case TextNodeType::HorizontalLine:
        appendLine(builder, {}, "---"_el);
        return;
    case TextNodeType::None:
    case TextNodeType::_count:
        return;
    default:
        auto line = AnyStringBuilder{};
        appendInline(line, *node);
        appendLine(builder, {}, line.toString());
        return;
    }
}

void PlainTextRenderer::renderChildren(AnyStringBuilder &builder, const TextNode &node) {
    auto inDiagnosticSection = false;
    for (const auto &child : node.children()) {
        if (child->type() == TextNodeType::Heading) {
            inDiagnosticSection = false;
        }
        const auto previousIndent = _indent;
        if (isRootDiagnosticContent(*child) || inDiagnosticSection) {
            _indent += CpLength{2U};
        }
        renderNode(builder, child);
        _indent = previousIndent;
        if (isDiagnosticSectionHeading(*child)) {
            inDiagnosticSection = true;
        }
    }
}

void PlainTextRenderer::renderCodeSnippet(AnyStringBuilder &builder, const TextNode &node) {
    for (const auto &line : node.children()) {
        if (line->type() != TextNodeType::CodeLine) {
            continue;
        }
        auto number = String{};
        auto source = String{};
        auto markers = std::vector<CodeSnippetLayoutMarker>{};
        for (const auto &child : line->children()) {
            if (child->type() == TextNodeType::CodeLineNumber) {
                number = nodeText(*child);
            } else if (child->type() == TextNodeType::CodeLineText) {
                source = nodeText(*child);
            } else if (child->type() == TextNodeType::CodeLineMarker && child->data() != nullptr) {
                if (const auto markerData = std::dynamic_pointer_cast<const CodeLineMarkerData>(child->data());
                    markerData != nullptr) {
                    markers.push_back(CodeSnippetLayoutMarker{markerData->range(), nodeText(*child)});
                }
            }
        }

        auto gutter = String{};
        auto markerGutter = String{};
        if (!number.isEmpty()) {
            auto numberBuilder = AnyStringBuilder{};
            const auto padding =
                std::max(cCodeLineNumberWidth - static_cast<int>(number.characterLength().toSizeT()), 0);
            numberBuilder.append(Char{U' '}, CpLength::fromSizeT(static_cast<std::size_t>(padding)));
            numberBuilder.append(number);
            numberBuilder.append(" │ "_el);
            gutter = numberBuilder.toString();
            auto markerBuilder = AnyStringBuilder{};
            markerBuilder.append(Char{U' '}, CpLength::fromSizeT(cCodeLineNumberWidth));
            markerBuilder.append(" │ "_el);
            markerGutter = markerBuilder.toString();
        }
        const auto sourceWidth =
            std::max(cPlainCodeSnippetWidth - static_cast<int>(gutter.characterLength().toSizeT()), 1);
        const auto layout = CodeSnippetLayout{source, markers, sourceWidth};
        const auto &rows = layout.rows();
        for (auto rowIndex = std::size_t{0}; rowIndex < rows.size(); ++rowIndex) {
            auto rowBuilder = AnyStringBuilder{};
            for (const auto &cell : rows[rowIndex].cells()) {
                rowBuilder.append(cell.text);
            }
            appendLine(builder, rowIndex == 0 ? gutter : markerGutter, rowBuilder.toString());

            for (auto markerIndex = std::size_t{0}; markerIndex < markers.size(); ++markerIndex) {
                const auto &marker = markers[markerIndex];
                const auto placement = rows[rowIndex].markerPlacement(marker.range);
                if (!placement.has_value()) {
                    continue;
                }
                auto markerBuilder = AnyStringBuilder{};
                markerBuilder.append(Char{U' '}, CpLength::fromSizeT(static_cast<std::size_t>(placement->start)));
                const auto point = marker.range.isEmpty();
                markerBuilder.append(
                    Char{point ? U'↑' : U'▔'}, CpLength::fromSizeT(static_cast<std::size_t>(placement->length)));
                const auto label = marker.label;
                const auto labelWidth = label.characterLength().toSizeT() + 3U;
                if (layout.isLastMarkerRow(rowIndex, marker.range) && !label.isEmpty() &&
                    markerBuilder.length().toSizeT() + labelWidth <= static_cast<std::size_t>(sourceWidth)) {
                    markerBuilder.append(" ("_el).append(label).append(")"_el);
                }
                appendLine(builder, markerGutter, markerBuilder.toString());
            }
        }
    }
}

void PlainTextRenderer::renderTermList(AnyStringBuilder &builder, const TextNode &node) {
    const auto descriptionColumn = termListDescriptionColumn(node);
    for (const auto &child : node.children()) {
        if (child->type() == TextNodeType::TermItem || child->type() == TextNodeType::FieldItem) {
            renderTermItem(builder, *child, descriptionColumn);
        }
    }
}

void PlainTextRenderer::renderTermItem(
    AnyStringBuilder &builder, const TextNode &node, const CpLength descriptionColumn) {
    auto name = String{};
    auto description = String{};
    for (const auto &child : node.children()) {
        if (child->type() == TextNodeType::TermName || child->type() == TextNodeType::FieldLabel) {
            name = termName(*child);
        } else if (child->type() == TextNodeType::TermDescription || child->type() == TextNodeType::FieldContent) {
            description = termDescription(*child);
        }
    }

    if (description.isEmpty()) {
        appendLine(builder, {}, name);
    } else {
        auto prefix = AnyStringBuilder{};
        prefix.append(name);
        const auto nameLength = name.characterLength();
        if (nameLength < descriptionColumn) {
            prefix.append(Char{U' '}, descriptionColumn - nameLength);
        } else {
            appendLine(builder, {}, name);
            prefix.append(Char{U' '}, descriptionColumn);
        }
        auto continuation = AnyStringBuilder{};
        continuation.append(Char{U' '}, descriptionColumn);
        appendWrappedLine(builder, prefix.toString(), continuation.toString(), description);
    }

    const auto previousIndent = _indent;
    _indent += CpLength{2U};
    for (const auto &child : node.children()) {
        if (child->type() == TextNodeType::TermList || child->type() == TextNodeType::FieldList) {
            renderTermList(builder, *child);
        }
    }
    _indent = previousIndent;
}

auto PlainTextRenderer::termName(const TextNode &node) -> String {
    auto builder = AnyStringBuilder{};
    auto firstOptionName = true;
    const auto usesOptionNames = node.contains(TextNodeType::OptionName);
    for (const auto &child : node.children()) {
        if (usesOptionNames && child->type() == TextNodeType::OptionName) {
            if (firstOptionName) {
                if (!child->contains(TextNodeType::OptionShort)) {
                    builder.append("    "_el);
                }
            } else {
                builder.append(", "_el);
            }
            appendInline(builder, *child);
            firstOptionName = false;
            continue;
        }
        if (!usesOptionNames || child->type() != TextNodeType::OptionName) {
            appendInline(builder, *child);
        }
    }
    if (node.style().contains("option-label"_el) || node.type() == TextNodeType::FieldLabel) {
        builder.append(U':');
    }
    return builder.toString();
}

auto PlainTextRenderer::termDescription(const TextNode &node) -> String {
    auto builder = AnyStringBuilder{};
    for (const auto &child : node.children()) {
        if (child->type().renderClass() == TextNodeType::RenderClass::Structure) {
            continue;
        }
        if (!builder.isEmpty() && child->type() == TextNodeType::OptionDetails) {
            builder.append(U' ');
        }
        appendInline(builder, *child);
    }
    return builder.toString();
}

auto PlainTextRenderer::termListDescriptionColumn(const TextNode &node) -> CpLength {
    auto widest = CpLength{};
    if (node.type() == TextNodeType::TermItem || node.type() == TextNodeType::FieldItem) {
        for (const auto &child : node.children()) {
            if (child->type() == TextNodeType::TermName || child->type() == TextNodeType::FieldLabel) {
                widest = std::max(widest, termName(*child).characterLength());
            }
        }
    } else {
        for (const auto &child : node.children()) {
            if (child->type() == TextNodeType::TermItem || child->type() == TextNodeType::FieldItem) {
                for (const auto &termChild : child->children()) {
                    if (termChild->type() == TextNodeType::TermName || termChild->type() == TextNodeType::FieldLabel) {
                        widest = std::max(widest, termName(*termChild).characterLength());
                    }
                }
            }
        }
    }
    if (node.type() == TextNodeType::FieldList || node.type() == TextNodeType::FieldItem) {
        return widest + CpLength::one();
    }
    return std::clamp(widest + CpLength{2U}, cTermDescriptionMinimumColumn, cTermDescriptionMaximumColumn);
}

void PlainTextRenderer::renderList(AnyStringBuilder &builder, const TextNode &node, const bool numbered) {
    auto index = std::size_t{1U};
    for (const auto &child : node.children()) {
        const auto prefix = listPrefix(index, numbered);
        if (child->type().isListItem()) {
            renderListItem(builder, *child, prefix);
            ++index;
            continue;
        }
        appendLine(builder, prefix, {});
        const auto previousIndent = _indent;
        _indent += CpLength{2U};
        renderNode(builder, child);
        _indent = previousIndent;
    }
}

void PlainTextRenderer::renderListItem(AnyStringBuilder &builder, const TextNode &node, const String &prefix) {
    auto line = AnyStringBuilder{};
    if (!node.text().isEmpty()) {
        line.append(node.text());
    }
    for (const auto &child : node.children()) {
        if (!isNestedBlock(*child)) {
            appendInline(line, *child);
        }
    }
    appendLine(builder, prefix, line.toString());

    const auto previousIndent = _indent;
    _indent += CpLength{2U};
    for (const auto &child : node.children()) {
        if (isNestedBlock(*child)) {
            renderNode(builder, child);
        }
    }
    _indent = previousIndent;
}

void PlainTextRenderer::appendInline(AnyStringBuilder &builder, const TextNode &node) {
    switch (node.type().raw()) {
    case TextNodeType::LineBreak:
        builder.append(U'\n');
        return;
    case TextNodeType::HorizontalLine:
        builder.append("---"_el);
        return;
    case TextNodeType::OptionMeta:
        appendPlaceholder(builder, node, "<"_el, ">"_el);
        return;
    case TextNodeType::OptionOptional:
        appendPlaceholder(builder, node, "["_el, "]"_el);
        return;
    case TextNodeType::None:
    case TextNodeType::_count:
        return;
    default:
        if (node.type() == TextNodeType::Heading && node.level() == 1 && node.style().contains("diagnostic-title"_el) &&
            node.style().contains("error"_el) && node.parent() != nullptr &&
            node.parent()->type() == TextNodeType::Document) {
            builder.append("Error: "_el);
        }
        if (!node.text().isEmpty()) {
            builder.append(node.text());
        }
        appendInlineChildren(builder, node);
        if (node.type() == TextNodeType::Heading &&
            (node.style().contains("option-section"_el) || node.style().contains("diagnostic-section"_el) ||
                node.style().contains("diagnostic-cause"_el) || node.style().contains("diagnostic-full-help"_el))) {
            builder.append(U':');
        }
        return;
    }
}

void PlainTextRenderer::appendPlaceholder(
    AnyStringBuilder &builder, const TextNode &node, const String &prefix, const String &suffix) {
    builder.append(prefix);
    if (!node.text().isEmpty()) {
        builder.append(node.text());
    }
    appendInlineChildren(builder, node);
    builder.append(suffix);
}

void PlainTextRenderer::appendInlineChildren(AnyStringBuilder &builder, const TextNode &node) {
    for (const auto &child : node.children()) {
        appendInline(builder, *child);
    }
}

void PlainTextRenderer::appendNodeText(AnyStringBuilder &builder, const TextNode &node) {
    if (!node.text().isEmpty()) {
        builder.append(node.text());
    }
    for (const auto &child : node.children()) {
        appendNodeText(builder, *child);
    }
}

auto PlainTextRenderer::nodeText(const TextNode &node) -> String {
    auto builder = AnyStringBuilder{};
    appendNodeText(builder, node);
    return builder.toString();
}

void PlainTextRenderer::appendLine(AnyStringBuilder &builder, const String &prefix, const String &text) {
    if (!_firstLine) {
        builder.append(U'\n');
    }
    _firstLine = false;
    appendIndent(builder, _indent);
    if (!prefix.isEmpty()) {
        builder.append(prefix);
    }
    if (!text.isEmpty()) {
        builder.append(text);
    }
}

void PlainTextRenderer::appendWrappedLine(
    AnyStringBuilder &builder, const String &prefix, const String &continuation, const String &text) {
    auto reader = StringCharReader{text};
    auto line = AnyStringBuilder::basedOn(prefix);
    auto firstWord = true;
    auto continuationLine = false;

    while (!reader.isAtEnd()) {
        reader.advanceWhile(AsciiCategory::Whitespace);

        auto wordLength = CpLength{};
        reader.startCapture();
        reader.readUntil(
            [&](Char) -> util::LoopStatus {
                ++wordLength;
                return util::LoopStatus::Continue;
            },
            AsciiCategory::Whitespace);
        if (wordLength.isZero()) {
            break;
        }

        const auto separatorLength = firstWord ? CpLength{} : CpLength::one();
        if (!firstWord && line.length() + separatorLength + wordLength > cPlainTermListWidth) {
            appendLine(builder, {}, line.toString());
            line.clear();
            line.append(continuation);
            firstWord = true;
            continuationLine = true;
        }
        if (!firstWord) {
            line.append(U' ');
        }
        line.append(reader.takeCapture().toU8String());
        firstWord = false;
    }
    if (!firstWord || !continuationLine) {
        appendLine(builder, {}, line.toString());
    }
}

void PlainTextRenderer::appendIndent(AnyStringBuilder &builder, const CpLength indent) {
    if (!indent.isZero()) {
        builder.append(Char{U' '}, indent);
    }
}

auto PlainTextRenderer::listPrefix(const std::size_t index, const bool numbered) -> String {
    if (!numbered) {
        return "- "_el;
    }
    return String::fromJoined({String::fromInteger(index), ". "_el});
}

auto PlainTextRenderer::isNestedBlock(const TextNode &node) noexcept -> bool {
    return node.type() == TextNodeType::BulletList || node.type() == TextNodeType::NumberedList ||
        node.type() == TextNodeType::DefinitionList || node.type() == TextNodeType::TermList ||
        node.type() == TextNodeType::FieldList || node.type() == TextNodeType::Section ||
        node.type() == TextNodeType::Blockquote || node.type() == TextNodeType::CodeSnippet;
}

auto PlainTextRenderer::isDiagnosticSectionHeading(const TextNode &node) noexcept -> bool {
    return node.type() == TextNodeType::Heading && node.style().contains("diagnostic-section"_el);
}

auto PlainTextRenderer::isRootDiagnosticContent(const TextNode &node) const noexcept -> bool {
    if (!_diagnosticDocument || node.parent() == nullptr || node.parent()->type() != TextNodeType::Document) {
        return false;
    }
    return node.type() == TextNodeType::Paragraph || node.type() == TextNodeType::FieldList ||
        node.type() == TextNodeType::TermList;
}

}
