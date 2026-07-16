// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../error/InternalError.hpp"
#include "../node_data/Anchor.hpp"
#include "../node_data/CharacterCategory.hpp"
#include "../node_data/CharacterClass.hpp"
#include "../node_data/CharacterSequence.hpp"
#include "../node_data/Group.hpp"
#include "../node_data/Quantifier.hpp"
#include "../node_data/Sequence.hpp"

#include <functional>
#include <memory>
#include <ranges>
#include <variant>
#include <vector>

namespace erbsland::re::impl {

/// A unique ID for each pattern node.
/// Root node always have ID 0.
using PatternNodeId = uint32_t;

/// A single node in the AST of a parsed regular expression.
class PatternNode : public std::enable_shared_from_this<PatternNode> {
public:
    using Data = std::variant<
        node_data::Group,
        node_data::Sequence,
        node_data::Anchor,
        node_data::CharacterCategory,
        node_data::CharacterSequence,
        node_data::CharacterClass,
        node_data::Quantifier>;

public:
    /// Create a new node from the given data structure.
    /// @param id The unique node identifier.
    /// @param data The data structure.
    constexpr explicit PatternNode(const PatternNodeId id, Data &&data) noexcept : _id{id}, _data(std::move(data)) {}

public: // tests
    /// Test if this is a group node.
    [[nodiscard]] constexpr auto isGroup() const noexcept -> bool {
        return std::holds_alternative<node_data::Group>(_data);
    }
    /// Test if this is a sequence of nodes.
    [[nodiscard]] constexpr auto isSequence() const noexcept -> bool {
        return std::holds_alternative<node_data::Sequence>(_data);
    }
    /// Test if this is a character sequence.
    [[nodiscard]] constexpr auto isCharacterSequence() const noexcept -> bool {
        return std::holds_alternative<node_data::CharacterSequence>(_data);
    }
    /// Test if this is a character range.
    [[nodiscard]] constexpr auto isCharacterClass() const noexcept -> bool {
        return std::holds_alternative<node_data::CharacterClass>(_data);
    }
    /// Test if this is a quantifier node.
    [[nodiscard]] constexpr auto isQuantifier() const noexcept -> bool {
        return std::holds_alternative<node_data::Quantifier>(_data);
    }

public: // accessors
    /// Access the ID.
    [[nodiscard]] constexpr auto id() const noexcept -> PatternNodeId { return _id; }
    /// Access the parent.
    [[nodiscard]] auto parent() const noexcept -> PatternNodePtr { return _parent.lock(); }
    /// Access the raw data.
    [[nodiscard]] auto data() const noexcept -> const Data & { return _data; }
    /// Access the raw data.
    [[nodiscard]] auto data() noexcept -> Data & { return _data; }
    /// Access the child/children of this node.
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> {
        return std::visit([](const auto &data) -> std::span<const PatternNodePtr> { return data.children(); }, _data);
    }
    /// Access the size of this nodes content/children.
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return std::visit([](const auto &data) -> std::size_t { return data.size(); }, _data);
    }
    /// Test if this node is empty (size == 0)
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return size() == 0; }

public: // modifiers
    /// Set a new parent.
    void setParent(const PatternNodePtr &parent) noexcept { _parent = parent; }

public: // helper methods
    /// Add a child to this node.
    void addChild(const PatternNodePtr &node);
    /// Replace the last child with the given node.
    void replaceLastChild(const PatternNodePtr &node);
    /// Append a character to this character sequence.
    void addCharacter(text::Char character);
    /// Traverse the node-tree and call the given function for each node.
    void traverse(const std::function<void(PatternNode &, int)> &visitor);
    /// Traverse the node-tree and call the given function for each node.
    void traverse(const std::function<void(const PatternNode &, int)> &visitor) const;

    /// Traverse the node-tree and call `onEnter` when a node is first visited (root-to-leaf)
    /// and `onExit` after all children were visited (leaf-to-root).
    ///
    /// This traversal is non-recursive.
    void traverse(
        const std::function<void(PatternNode &, int)> &onEnter, const std::function<void(PatternNode &, int)> &onExit);

    /// Traverse the node-tree and call `onEnter` when a node is first visited (root-to-leaf)
    /// and `onExit` after all children were visited (leaf-to-root).
    ///
    /// This traversal is non-recursive.
    void traverse(
        const std::function<void(const PatternNode &, int)> &onEnter,
        const std::function<void(const PatternNode &, int)> &onExit) const;

public: // inspection
    /// Create a well-defined debug string that can be used to validate generated node trees in unit tests.
    [[nodiscard]] auto toTestString() const -> text::String;

    /// Create a vector of debug strings with the full node-tree, starting with this one.
    /// Each level of the tree is indented with two space characters.
    /// @return A vector of debug strings.
    [[nodiscard]] auto toTestTree() const -> text::StringViewList;

private:
    PatternNodeId _id;                  ///< The unique ID of this node.
    std::weak_ptr<PatternNode> _parent; ///< The parent of this node.
    Data _data;                         ///< The data of this node.
};

}
