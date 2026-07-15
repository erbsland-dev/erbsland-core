// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextNodeType.hpp"

#include "Literals.hpp"

namespace erbsland::text {

using namespace literals;

auto TextNodeType::toString() const noexcept -> StringView {
    switch (_value) {
    case Document:
        return "Document"_el;
    case Paragraph:
        return "Paragraph"_el;
    case Section:
        return "Section"_el;
    case Blockquote:
        return "Blockquote"_el;
    case LineBreak:
        return "LineBreak"_el;
    case Heading:
        return "Heading"_el;
    case BulletList:
        return "BulletList"_el;
    case NumberedList:
        return "NumberedList"_el;
    case BulletListItem:
        return "BulletListItem"_el;
    case NumberedListItem:
        return "NumberedListItem"_el;
    case DefinitionList:
        return "DefinitionList"_el;
    case DefinitionTerm:
        return "DefinitionTerm"_el;
    case DefinitionDescription:
        return "DefinitionDescription"_el;
    case TermList:
        return "TermList"_el;
    case TermItem:
        return "TermItem"_el;
    case TermName:
        return "TermName"_el;
    case TermDescription:
        return "TermDescription"_el;
    case FieldList:
        return "FieldList"_el;
    case FieldItem:
        return "FieldItem"_el;
    case FieldLabel:
        return "FieldLabel"_el;
    case FieldContent:
        return "FieldContent"_el;
    case CodeBlock:
        return "CodeBlock"_el;
    case CodeSnippet:
        return "CodeSnippet"_el;
    case CodeLine:
        return "CodeLine"_el;
    case CodeLineNumber:
        return "CodeLineNumber"_el;
    case CodeLineText:
        return "CodeLineText"_el;
    case CodeLineMarker:
        return "CodeLineMarker"_el;
    case HorizontalLine:
        return "HorizontalLine"_el;
    case Text:
        return "Text"_el;
    case Emphasis:
        return "Emphasis"_el;
    case Strong:
        return "Strong"_el;
    case Underline:
        return "Underline"_el;
    case Span:
        return "Span"_el;
    case Link:
        return "Link"_el;
    case Code:
        return "Code"_el;
    case OptionExecutable:
        return "OptionExecutable"_el;
    case OptionModule:
        return "OptionModule"_el;
    case OptionName:
        return "OptionName"_el;
    case OptionShort:
        return "OptionShort"_el;
    case OptionLong:
        return "OptionLong"_el;
    case OptionMeta:
        return "OptionMeta"_el;
    case OptionOptional:
        return "OptionOptional"_el;
    case OptionDetails:
        return "OptionDetails"_el;
    case Separator:
        return "Separator"_el;
    case EscapeSequence:
        return "EscapeSequence"_el;
    case Unsupported:
        return "Unsupported"_el;
    case Error:
        return "Error"_el;
    case None:
        return "None"_el;
    case _count:
        break;
    }
    return "Unknown"_el;
}

auto TextNodeType::renderClass() const noexcept -> RenderClass {
    switch (_value) {
    case Document:
    case Section:
    case Blockquote:
    case BulletList:
    case NumberedList:
    case DefinitionList:
    case TermList:
    case FieldList:
    case CodeSnippet:
        return RenderClass::Structure;
    case Paragraph:
    case Heading:
    case BulletListItem:
    case NumberedListItem:
    case DefinitionTerm:
    case DefinitionDescription:
    case TermItem:
    case TermName:
    case TermDescription:
    case FieldItem:
    case FieldLabel:
    case FieldContent:
    case CodeBlock:
    case CodeLine:
    case HorizontalLine:
    case Unsupported:
    case Error:
        return RenderClass::Block;
    case Text:
    case Emphasis:
    case Strong:
    case Underline:
    case Span:
    case Link:
    case Code:
    case CodeLineNumber:
    case CodeLineText:
    case CodeLineMarker:
    case OptionExecutable:
    case OptionModule:
    case OptionName:
    case OptionShort:
    case OptionLong:
    case OptionMeta:
    case OptionOptional:
    case OptionDetails:
    case Separator:
    case EscapeSequence:
        return RenderClass::Inline;
    case LineBreak:
    case None:
    case _count:
        break;
    }
    return RenderClass::Empty;
}

auto TextNodeType::isInline() const noexcept -> bool {
    return renderClass() == RenderClass::Inline;
}

auto TextNodeType::isTextContainer() const noexcept -> bool {
    return _value == Paragraph || _value == Heading || isListItem() || _value == DefinitionTerm ||
        _value == DefinitionDescription || isTermListElement() || isFieldListElement() || _value == CodeBlock ||
        isInline() || _value == CodeSnippet || _value == CodeLine;
}

auto TextNodeType::isListContainer() const noexcept -> bool {
    return _value == BulletList || _value == NumberedList || _value == TermList || _value == FieldList;
}

auto TextNodeType::isListItem() const noexcept -> bool {
    return _value == BulletListItem || _value == NumberedListItem;
}

auto TextNodeType::isTermListElement() const noexcept -> bool {
    return _value == TermItem || _value == TermName || _value == TermDescription;
}

auto TextNodeType::isFieldListElement() const noexcept -> bool {
    return _value == FieldItem || _value == FieldLabel || _value == FieldContent;
}

auto TextNodeType::preserveWhitespace() const noexcept -> bool {
    return _value == CodeBlock || _value == CodeLine;
}

}
