// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

namespace erbsland::re::impl::node_data {

class Sequence : public NodeData {
public:
    /// Default constructor for fast zero-initialization.
    Sequence() = default;
    ~Sequence() = default;

public:
    /// Append a child node to the sequence.
    /// @param node The child to append.
    void addNode(const PatternNodePtr &node) { sequence.push_back(node); }
    /// Replace the last node in this sequence.
    void replaceLastNode(const PatternNodePtr &node) { sequence.back() = node; }
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        return text::StringFormat{"Sequence(size={})"}.build(sequence.size());
    }
    /// Access the children of this node as a zero‑overhead view.
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> { return sequence; }
    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return sequence.size(); }

public:
    /// The ordered list of child nodes.
    std::vector<PatternNodePtr> sequence;
};

}
