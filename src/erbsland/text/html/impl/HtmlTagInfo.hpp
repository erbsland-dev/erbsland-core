// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../String.hpp"
#include "../../TextNode.hpp"

#include <cstdint>

namespace erbsland::text::html::impl {

/// Static information for one supported or known HTML tag.
/// @tested{HtmlParserTest}
class HtmlTagInfo final {
public:
    enum class NodeBehavior : uint8_t { Inline, Block };
    enum class LevelMode : uint8_t { Fixed, UseListLevel };
    enum class SubtreeBehavior : uint8_t { Keep, Suppress };
    enum class Transparency : uint8_t { Transparent };

    HtmlTagInfo() = default;

    /// Create one node-producing tag description.
    /// @param nodeType The mapped node type.
    /// @param nodeBehavior Whether the tag starts a block node.
    HtmlTagInfo(TextNode::Type nodeType, NodeBehavior nodeBehavior = NodeBehavior::Inline) noexcept :
        _nodeType{nodeType}, _hasNodeType{true}, _isBlock{nodeBehavior == NodeBehavior::Block} {}

    /// Create one node-producing tag description with an explicit level.
    /// @param nodeType The mapped node type.
    /// @param level The configured heading level.
    /// @param nodeBehavior Whether the tag starts a block node.
    HtmlTagInfo(TextNode::Type nodeType, TextNode::Level level, NodeBehavior nodeBehavior) noexcept :
        _nodeType{nodeType}, _level{level}, _hasNodeType{true}, _isBlock{nodeBehavior == NodeBehavior::Block} {}

    /// Create one node-producing tag description that uses the current list level.
    /// @param nodeType The mapped node type.
    /// @param levelMode Whether the node level is derived from the open list depth.
    /// @param nodeBehavior Whether the tag starts a block node.
    HtmlTagInfo(TextNode::Type nodeType, LevelMode levelMode, NodeBehavior nodeBehavior) noexcept :
        _nodeType{nodeType},
        _hasNodeType{true},
        _usesListLevel{levelMode == LevelMode::UseListLevel},
        _isBlock{nodeBehavior == NodeBehavior::Block} {}

    /// Create one placeholder tag description.
    /// @param placeholderText The placeholder text for unsupported tags.
    /// @param subtreeBehavior Whether nested content is ignored.
    explicit HtmlTagInfo(String placeholderText, SubtreeBehavior subtreeBehavior = SubtreeBehavior::Keep) noexcept :
        _placeholderText{std::move(placeholderText)}, _suppressSubtree{subtreeBehavior == SubtreeBehavior::Suppress} {}

    /// Create one transparent tag description.
    /// @param transparency Marker that this tag is transparent.
    /// @param subtreeBehavior Whether nested content is ignored.
    explicit HtmlTagInfo(Transparency transparency, SubtreeBehavior subtreeBehavior = SubtreeBehavior::Keep) noexcept :
        _transparent{transparency == Transparency::Transparent},
        _suppressSubtree{subtreeBehavior == SubtreeBehavior::Suppress} {}

public:
    /// Access the mapped node type.
    [[nodiscard]] auto nodeType() const noexcept -> TextNode::Type { return _nodeType; }
    /// Access the configured heading level.
    [[nodiscard]] auto level() const noexcept -> TextNode::Level { return _level; }
    /// Access the placeholder text for unsupported tags.
    [[nodiscard]] auto placeholderText() const noexcept -> const String & { return _placeholderText; }
    /// Test if the tag creates a node.
    [[nodiscard]] auto hasNodeType() const noexcept -> bool { return _hasNodeType; }
    /// Test if the list nesting defines the node level.
    [[nodiscard]] auto usesListLevel() const noexcept -> bool { return _usesListLevel; }
    /// Test if the tag starts a block node.
    [[nodiscard]] auto isBlock() const noexcept -> bool { return _isBlock; }
    /// Test if the tag itself is ignored.
    [[nodiscard]] auto transparent() const noexcept -> bool { return _transparent; }
    /// Test if all nested content is ignored.
    [[nodiscard]] auto suppressSubtree() const noexcept -> bool { return _suppressSubtree; }

private:
    TextNode::Type _nodeType{TextNode::Type::Document}; ///< The mapped node type.
    TextNode::Level _level{0};                          ///< The configured heading level.
    String _placeholderText{};                          ///< Placeholder text for unsupported tags.
    bool _hasNodeType{false};                           ///< `true` if the tag creates a node.
    bool _usesListLevel{false};                         ///< `true` if the list nesting defines the node level.
    bool _isBlock{false};                               ///< `true` if the tag starts a block node.
    bool _transparent{false};                           ///< `true` if the tag itself is ignored.
    bool _suppressSubtree{false};                       ///< `true` if all nested content is ignored.
};

}
