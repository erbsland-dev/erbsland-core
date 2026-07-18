// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

#include "../text/CharClass.hpp"

namespace erbsland::re::impl::node_data {

/// A custom character range.
class CharacterClass : public NodeData {
public:
    /// Fast constructor.
    /// @param characterClass The set of character ranges represented by this node.
    /// @param isNegated If this character range is negated.
    template <typename Fwd>
        requires std::is_base_of_v<CharClass, std::remove_cvref_t<Fwd>>
    explicit CharacterClass(Fwd &&characterClass, const bool isNegated) noexcept :
        characterClass{std::forward<Fwd>(characterClass)}, isNegated{isNegated} {}

public:
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        using namespace text::literals;
        return text::StringFormat{"CharacterClass([{}{}])"}.build(
            isNegated ? "^"_el : text::String{}, characterClass.toString());
    }

    /// Access the children of this node as a zero‑overhead view (always empty for leaves).
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> { return {}; }

    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return characterClass.size(); }

public:
    /// The set of allowed characters represented as ranges.
    CharClass characterClass;
    /// If the character range is negated.
    bool isNegated{false};
    /// The index of the character range in the data block.
    /// -1 = not in the data block.
    int32_t dataIndex{-1};
};

}
