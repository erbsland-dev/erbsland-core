// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

#include "../text/TextAnchor.hpp"

namespace erbsland::re::impl::node_data {

/// An anchor
class Anchor : public NodeData {
public:
    /// Fast constructor.
    /// @param textAnchor The text anchor
    explicit Anchor(const TextAnchor textAnchor) noexcept : textAnchor{textAnchor} {}

public:
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        return text::StringFormat{"Anchor({})"}.build(textAnchor.toString());
    }

    /// Access the children of this node as a zero‑overhead view (always empty for leaves).
    [[nodiscard, maybe_unused]] auto children() const noexcept -> std::span<const PatternNodePtr> { return {}; }

    /// Access the size of this data block.
    [[nodiscard, maybe_unused]] auto size() const noexcept -> std::size_t { return 0; }

public:
    TextAnchor textAnchor; ///< The text anchor.
};

}
