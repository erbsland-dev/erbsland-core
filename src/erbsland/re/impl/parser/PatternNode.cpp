// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PatternNode.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

void PatternNode::addChild(const PatternNodePtr &node) {
    node->setParent(shared_from_this());
    if (isSequence()) {
        std::get<node_data::Sequence>(_data).addNode(node);
    } else if (isGroup()) {
        std::get<node_data::Group>(_data).addNode(node);
    } else {
        throwInternalError("Cannot add a child to a non-sequence/alternative node"_el);
    }
}

void PatternNode::replaceLastChild(const PatternNodePtr &node) {
    if (isSequence()) {
        node->setParent(shared_from_this());
        std::get<node_data::Sequence>(_data).replaceLastNode(node);
    } else {
        throwInternalError("Cannot replace the last child of a non-sequence node"_el);
    }
}

void PatternNode::addCharacter(const text::Char character) {
    ERBSLAND_CORE_RE_REQUIRE_DEBUG(isCharacterSequence(), "Cannot append a character to a non-character sequence"_el);
    std::get<node_data::CharacterSequence>(_data).chars.append(character);
}

void PatternNode::traverse(const std::function<void(PatternNode &, int)> &visitor) {
    // Non-recursive pre-order traversal (children left-to-right).
    // Using raw-pointers is safe because the ownership/lifetime is managed by shared_ptr in the tree.
    struct StackItem {
        PatternNode *node;
        int depth;
    };
    std::vector<StackItem> stack;
    stack.push_back({.node = this, .depth = 0});
    while (!stack.empty()) {
        auto [node, depth] = stack.back();
        stack.pop_back();
        visitor(*node, depth);
        // Push children in reverse order to process left-to-right
        auto children = node->children();
        for (const auto &child : std::ranges::reverse_view(children)) {
            stack.push_back({.node = child.get(), .depth = depth + 1});
        }
    }
}

void PatternNode::traverse(const std::function<void(const PatternNode &, int)> &visitor) const {
    // Non-recursive pre-order traversal (children left-to-right).
    // Using raw-pointers is safe because the ownership/lifetime is managed by shared_ptr in the tree.
    struct StackItem {
        const PatternNode *node;
        int depth;
    };
    std::vector<StackItem> stack;
    stack.push_back({.node = this, .depth = 0});
    while (!stack.empty()) {
        auto [node, depth] = stack.back();
        stack.pop_back();
        visitor(*node, depth);
        auto children = node->children();
        for (const auto &it : std::ranges::reverse_view(children)) {
            stack.push_back({.node = it.get(), .depth = depth + 1});
        }
    }
}

void PatternNode::traverse(
    const std::function<void(PatternNode &, int)> &onEnter, const std::function<void(PatternNode &, int)> &onExit) {

    struct StackItem {
        PatternNode *node;
        std::size_t depth;
        std::size_t nextChildIndex;
        bool entered;
    };
    std::vector<StackItem> stack;
    stack.push_back({.node = this, .depth = 0, .nextChildIndex = 0, .entered = false});
    while (!stack.empty()) {
        auto &item = stack.back();
        if (!item.entered) {
            item.entered = true;
            onEnter(*item.node, static_cast<int>(item.depth));
        }

        const auto children = item.node->children();
        if (item.nextChildIndex < children.size()) {
            auto *child = children[item.nextChildIndex].get();
            ++item.nextChildIndex;
            stack.push_back({.node = child, .depth = item.depth + 1, .nextChildIndex = 0, .entered = false});
        } else {
            onExit(*item.node, static_cast<int>(item.depth));
            stack.pop_back();
        }
    }
}

void PatternNode::traverse(
    const std::function<void(const PatternNode &, int)> &onEnter,
    const std::function<void(const PatternNode &, int)> &onExit) const {

    struct StackItem {
        const PatternNode *node;
        std::size_t depth;
        std::size_t nextChildIndex;
        bool entered;
    };
    std::vector<StackItem> stack;
    stack.push_back({.node = this, .depth = 0, .nextChildIndex = 0, .entered = false});
    while (!stack.empty()) {
        auto &item = stack.back();
        if (!item.entered) {
            item.entered = true;
            onEnter(*item.node, static_cast<int>(item.depth));
        }

        const auto children = item.node->children();
        if (item.nextChildIndex < children.size()) {
            const auto *child = children[item.nextChildIndex].get();
            ++item.nextChildIndex;
            stack.push_back({.node = child, .depth = item.depth + 1, .nextChildIndex = 0, .entered = false});
        } else {
            onExit(*item.node, static_cast<int>(item.depth));
            stack.pop_back();
        }
    }
}

auto PatternNode::toTestString() const -> text::String {
    return std::visit([](const auto &data) -> text::String { return data.toTestString(); }, _data);
}

auto PatternNode::toTestTree() const -> text::StringViewList {
    text::StringViewList tree;
    traverse([&tree](const PatternNode &node, const int depth) -> void {
        auto line = text::String::fromCharacter(U' ', unit::CpLength{static_cast<std::uint32_t>(depth * 2)});
        line.append(node.toTestString());
        tree.append(std::move(line));
    });
    return tree;
}

}
