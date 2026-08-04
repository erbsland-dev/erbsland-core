// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/CharSet.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"
#include "../text/TextNodeType.hpp"

#include <initializer_list>
#include <optional>

namespace erbsland::cterm {

/// A selector used to define or request one terminal document style rule.
/// @tested{TerminalDocumentStyleTest}
class TerminalDocumentStyleSelector final {
public:
    using TokenList = text::StringList; ///< The normalized token list.

public:
    /// Create a paragraph selector.
    TerminalDocumentStyleSelector() noexcept = default;
    /// Create a selector for the given text node type.
    /// @param nodeType The node type to match.
    explicit TerminalDocumentStyleSelector(text::TextNodeType nodeType) noexcept : _nodeType{nodeType} {}
    /// Create a selector with an optional level.
    /// @param nodeType The node type to match.
    /// @param level The optional heading or list nesting level.
    TerminalDocumentStyleSelector(text::TextNodeType nodeType, std::optional<int> level) noexcept :
        _nodeType{nodeType}, _level{level} {}
    /// Create a selector with required style tokens.
    /// @param nodeType The node type to match.
    /// @param requiredStyleTokens Required style tokens parsed from `TextNode::style()`.
    TerminalDocumentStyleSelector(text::TextNodeType nodeType, std::initializer_list<text::String> requiredStyleTokens);
    /// Create a selector with an optional level and required style tokens.
    /// @param nodeType The node type to match.
    /// @param level The optional heading or list nesting level.
    /// @param requiredStyleTokens Required style tokens parsed from `TextNode::style()`.
    TerminalDocumentStyleSelector(
        text::TextNodeType nodeType, std::optional<int> level, std::initializer_list<text::String> requiredStyleTokens);
    /// Create a selector with an optional ancestor constraint.
    /// @param nodeType The node type to match.
    /// @param level The optional heading or list nesting level.
    /// @param requiredStyleTokens Required style tokens.
    /// @param ancestorType A type that must occur in the parent chain.
    TerminalDocumentStyleSelector(
        text::TextNodeType nodeType,
        std::optional<int> level,
        std::initializer_list<text::String> requiredStyleTokens,
        std::optional<text::TextNodeType> ancestorType);

    // defaults
    ~TerminalDocumentStyleSelector() = default;
    TerminalDocumentStyleSelector(const TerminalDocumentStyleSelector &) = default;
    TerminalDocumentStyleSelector(TerminalDocumentStyleSelector &&) = default;
    auto operator=(const TerminalDocumentStyleSelector &) -> TerminalDocumentStyleSelector & = default;
    auto operator=(TerminalDocumentStyleSelector &&) -> TerminalDocumentStyleSelector & = default;

public: // operators
    /// Compare two selectors for exact identity.
    [[nodiscard]] auto operator==(const TerminalDocumentStyleSelector &other) const -> bool {
        return _nodeType == other._nodeType && _level == other._level &&
            _requiredStyleTokens == other._requiredStyleTokens && _ancestorType == other._ancestorType;
    }
    /// Compare two selectors for exact inequality.
    [[nodiscard]] auto operator!=(const TerminalDocumentStyleSelector &other) const -> bool {
        return !(*this == other);
    }

public: // accessors
    /// Get the node type to match.
    [[nodiscard]] auto nodeType() const noexcept -> text::TextNodeType { return _nodeType; }
    /// Get the optional heading or list nesting level.
    [[nodiscard]] auto level() const noexcept -> const std::optional<int> & { return _level; }
    /// Get the required style tokens.
    [[nodiscard]] auto requiredStyleTokens() const noexcept -> const TokenList & { return _requiredStyleTokens; }
    /// Get the optional required ancestor type.
    [[nodiscard]] auto ancestorType() const noexcept -> const std::optional<text::TextNodeType> & {
        return _ancestorType;
    }

public: // factories
    /// Create a selector for a node type.
    /// @param nodeType The node type to match.
    [[nodiscard]] static auto node(text::TextNodeType nodeType) noexcept -> TerminalDocumentStyleSelector {
        return TerminalDocumentStyleSelector{nodeType};
    }
    /// Create a document selector.
    [[nodiscard]] static auto document() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Document);
    }
    /// Create a section selector.
    [[nodiscard]] static auto section() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Section);
    }
    /// Create a paragraph selector.
    [[nodiscard]] static auto paragraph() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Paragraph);
    }
    /// Create a heading selector.
    /// @param level The one-based heading level.
    [[nodiscard]] static auto heading(int level) noexcept -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::Heading, level};
    }
    /// Create a bullet-list selector.
    /// @param level The zero-based nesting level.
    [[nodiscard]] static auto bulletList(int level) noexcept -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::BulletList, level};
    }
    /// Create a numbered-list selector.
    /// @param level The zero-based nesting level.
    [[nodiscard]] static auto numberedList(int level) noexcept -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::NumberedList, level};
    }
    /// Create a bullet-list-item selector.
    /// @param level The zero-based nesting level.
    [[nodiscard]] static auto bulletListItem(int level) noexcept -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::BulletListItem, level};
    }
    /// Create a numbered-list-item selector.
    /// @param level The zero-based nesting level.
    [[nodiscard]] static auto numberedListItem(int level) noexcept -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::NumberedListItem, level};
    }
    /// Create a definition-list selector.
    [[nodiscard]] static auto definitionList() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::DefinitionList);
    }
    /// Create a definition-term selector.
    [[nodiscard]] static auto definitionTerm() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::DefinitionTerm);
    }
    /// Create a definition-description selector.
    [[nodiscard]] static auto definitionDescription() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::DefinitionDescription);
    }
    /// Create a field-list selector.
    [[nodiscard]] static auto fieldList() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::FieldList);
    }
    /// Create a field-item selector.
    [[nodiscard]] static auto fieldItem() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::FieldItem);
    }
    /// Create a field-label selector.
    [[nodiscard]] static auto fieldLabel() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::FieldLabel);
    }
    /// Create a field-content selector.
    [[nodiscard]] static auto fieldContent() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::FieldContent);
    }
    /// Create a blockquote selector.
    [[nodiscard]] static auto blockquote() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Blockquote);
    }
    /// Create a code-block selector.
    [[nodiscard]] static auto codeBlock() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::CodeBlock);
    }
    /// Create a horizontal-line selector.
    [[nodiscard]] static auto horizontalLine() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::HorizontalLine);
    }
    /// Create an emphasis selector.
    [[nodiscard]] static auto emphasis() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Emphasis);
    }
    /// Create a strong-text selector.
    [[nodiscard]] static auto strong() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Strong);
    }
    /// Create an underline selector.
    [[nodiscard]] static auto underline() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Underline);
    }
    /// Create a span selector with required style tokens.
    /// @param requiredStyleTokens Required style tokens.
    [[nodiscard]] static auto span(std::initializer_list<text::String> requiredStyleTokens)
        -> TerminalDocumentStyleSelector {
        return {text::TextNodeType::Span, requiredStyleTokens};
    }
    /// Create a link selector.
    [[nodiscard]] static auto link() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Link);
    }
    /// Create an inline-code selector.
    [[nodiscard]] static auto code() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Code);
    }
    /// Create an inline-separator selector.
    [[nodiscard]] static auto separator() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::Separator);
    }
    /// Create an escape-sequence selector.
    [[nodiscard]] static auto escapeSequence() noexcept -> TerminalDocumentStyleSelector {
        return node(text::TextNodeType::EscapeSequence);
    }
    /// Create a selector constrained to descendants of the given type.
    /// @param nodeType The node type to match.
    /// @param ancestorType A type that must occur in the parent chain.
    /// @param requiredStyleTokens Required style tokens.
    [[nodiscard]] static auto descendantOf(
        text::TextNodeType nodeType,
        text::TextNodeType ancestorType,
        std::initializer_list<text::String> requiredStyleTokens = {}) -> TerminalDocumentStyleSelector {
        return {nodeType, std::nullopt, requiredStyleTokens, ancestorType};
    }

public:
    /// Normalize a token list by sorting and removing duplicates.
    /// @param tokens The tokens to normalize in-place.
    static void normalizeTokens(TokenList &tokens);
    /// Split and normalize a `TextNode::style()` value.
    /// @param value The raw style value.
    /// @return The normalized tokens.
    [[nodiscard]] static auto splitStyleTokens(const text::String &value) -> TokenList;

private:
    /// Get the characters that separate style tokens.
    [[nodiscard]] static auto styleTokenSeparators() -> const text::CharSet &;

    text::TextNodeType _nodeType{text::TextNodeType::Paragraph}; ///< The node type to match.
    std::optional<int> _level;                                   ///< The optional level qualifier.
    TokenList _requiredStyleTokens;                              ///< Required style tokens.
    std::optional<text::TextNodeType> _ancestorType;             ///< Required ancestor type.
};

}
