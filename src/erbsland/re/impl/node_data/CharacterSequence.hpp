// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

#include "../text/CharSequence.hpp"

namespace erbsland::re::impl::node_data {

/// A sequence of characters.
class CharacterSequence : public NodeData {
public:
    /// Default constructor for fast zero-initialization.
    CharacterSequence() = default;
    /// Fast constructor.
    /// @param character Create a single character sequence (will be merged later.)
    explicit CharacterSequence(const text::Char character) noexcept { chars.append(character); }
    ~CharacterSequence() = default;

public:
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        text::String safeString;
        std::ranges::for_each(*chars.sequence(), [&safeString](const auto character) -> void {
            appendToSafeString(safeString, character);
        });
        return text::StringFormat{"CharacterSequence(\"{}\")"}.build(safeString);
    }

    /// Access the children of this node as a zero‑overhead view (always empty for leaves).
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> {
        return {}; // empty span, leaves have no children
    }

    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return chars.size(); }

public:
    /// The contiguous sequence of characters represented by this node.
    CharSequence chars;
    /// The index of the character sequence in the data block.
    /// -1 = not in the data block.
    int32_t dataIndex{-1};
};

}
