// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringView.hpp"

#include <cstdint>

namespace erbsland::text {

/// The semantic type of a text document node.
/// @tested{TextDocumentTest}
class TextNodeType {
public:
    /// The raw semantic node type.
    enum Value : uint8_t {
        Document,              ///< The root node of a document.
        Paragraph,             ///< A paragraph containing inline content.
        Section,               ///< A structural section containing blocks.
        Blockquote,            ///< A quoted block containing blocks.
        LineBreak,             ///< An explicit line break.
        Heading,               ///< A heading containing inline content.
        BulletList,            ///< A bullet list containing list items.
        NumberedList,          ///< A numbered list containing list items.
        BulletListItem,        ///< A bullet-list item containing inline content and nested blocks.
        NumberedListItem,      ///< A numbered-list item containing inline content and nested blocks.
        DefinitionList,        ///< A definition list containing terms and descriptions.
        DefinitionTerm,        ///< A definition term.
        DefinitionDescription, ///< A definition description.
        TermList,              ///< A semantic term list with names and descriptions.
        TermItem,              ///< A semantic term-list item.
        TermName,              ///< A semantic term name.
        TermDescription,       ///< A semantic term description.
        FieldList,             ///< A form-like list of labeled fields.
        FieldItem,             ///< One field-list item.
        FieldLabel,            ///< The label of a field.
        FieldContent,          ///< The primary content of a field.
        CodeBlock,             ///< A block of code.
        CodeSnippet,           ///< A line-oriented code snippet.
        CodeLine,              ///< A line in a code snippet.
        CodeLineNumber,        ///< A code snippet line number.
        CodeLineText,          ///< A code snippet source text.
        CodeLineMarker,        ///< A marker attached to one code snippet source line.
        HorizontalLine,        ///< A horizontal separator.
        Text,                  ///< Plain inline text.
        Emphasis,              ///< Emphasized inline content.
        Strong,                ///< Strong inline content.
        Underline,             ///< Underlined inline content.
        Span,                  ///< Generic inline span.
        Link,                  ///< Link inline content.
        Code,                  ///< Inline code.
        OptionExecutable,      ///< Option output executable name.
        OptionModule,          ///< Option output module name.
        OptionName,            ///< Option output complete option name.
        OptionShort,           ///< Option output short option name.
        OptionLong,            ///< Option output long option name.
        OptionMeta,            ///< Option output meta value.
        OptionOptional,        ///< Option output optional placeholder.
        OptionDetails,         ///< Option output inline details.
        Separator,             ///< A semantic inline separator and wrapping opportunity.
        EscapeSequence,        ///< An indivisible inline escape sequence.
        Unsupported,           ///< Unsupported content preserved as text.
        Error,                 ///< Error content preserved as text.
        None,                  ///< No node type.

        _count,
    };

    /// The broad rendering class of a node type.
    enum class RenderClass : uint8_t {
        Empty,     ///< Does not render visible content directly.
        Structure, ///< Contains structural children.
        Block,     ///< Renders as a block.
        Inline,    ///< Renders inline.
    };

public:
    /// Create the empty node type.
    constexpr TextNodeType() noexcept = default;
    /// Create a node type from a raw value.
    constexpr TextNodeType(Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~TextNodeType() = default;
    TextNodeType(const TextNodeType &) noexcept = default;
    TextNodeType(TextNodeType &&) noexcept = default;
    auto operator=(const TextNodeType &) noexcept -> TextNodeType & = default;
    auto operator=(TextNodeType &&) noexcept -> TextNodeType & = default;

public: // operators
    /// Test if two node types are equal.
    [[nodiscard]] constexpr auto operator==(const TextNodeType &other) const noexcept -> bool = default;
    /// Test if this node type is the given raw value.
    [[nodiscard]] constexpr auto operator==(Value value) const noexcept -> bool { return _value == value; }
    /// Test if this node type is not the given raw value.
    [[nodiscard]] constexpr auto operator!=(Value value) const noexcept -> bool { return _value != value; }

public: // accessors
    /// Get the raw value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Value { return _value; }
    /// Get the display name of this node type.
    [[nodiscard]] auto toString() const noexcept -> StringView;
    /// Get the broad rendering class of this node type.
    [[nodiscard]] auto renderClass() const noexcept -> RenderClass;
    /// Test if this type renders inline.
    [[nodiscard]] auto isInline() const noexcept -> bool;
    /// Test if this type can directly contain text while parsing or building documents.
    [[nodiscard]] auto isTextContainer() const noexcept -> bool;
    /// Test if this type is a list container.
    [[nodiscard]] auto isListContainer() const noexcept -> bool;
    /// Test if this type is a list item.
    [[nodiscard]] auto isListItem() const noexcept -> bool;
    /// Test if this type is a term-list element.
    [[nodiscard]] auto isTermListElement() const noexcept -> bool;
    /// Test if this type is a field-list element.
    [[nodiscard]] auto isFieldListElement() const noexcept -> bool;
    /// Test if this type preserves whitespace when rendered as a paragraph-like block.
    [[nodiscard]] auto preserveWhitespace() const noexcept -> bool;

private:
    Value _value{None}; ///< The raw node type.
};

}
