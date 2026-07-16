// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NodeData.hpp"

#include "../text/Category.hpp"

namespace erbsland::re::impl::node_data {

/// A predefined character category.
class CharacterCategory : public NodeData {
public:
    /// Fast constructor.
    /// @param category The character category
    /// @param isNegated If the character category is negated NOT(A)
    explicit CharacterCategory(const Category category, const bool isNegated) noexcept :
        categories{{category}}, isNegated(isNegated) {}
    /// Fast constructor.
    /// @param categories A list of character categories (OR)
    /// @param isNegated If the character categories are negated NOT(A OR B OR C)
    explicit CharacterCategory(std::vector<Category> categories, const bool isNegated) noexcept :
        categories{std::move(categories)}, isNegated(isNegated) {}

public:
    /// Create a stable string used for validating node trees in tests.
    [[nodiscard]] auto toTestString() const -> text::String {
        using namespace text::literals;
        text::String categoriesString;
        if (isNegated) {
            categoriesString.append(U'^');
        }
        bool first = true;
        if (categories.size() > 1) {
            categoriesString.append(U'(');
        }
        for (const auto &category : categories) {
            if (!first) {
                categoriesString.append(U'|');
            }
            categoriesString.append(category.toLongString());
            first = false;
        }
        if (categories.size() > 1) {
            categoriesString.append(U')');
        }
        return text::StringFormat{"CharacterCategory({})"}.build(categoriesString);
    }

    /// Access the children of this node as a zero‑overhead view (always empty for leaves).
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> { return {}; }

    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return categories.size(); }

public:
    std::vector<Category> categories; ///< The character categories (OR).
    bool isNegated{false};            ///< If the character category is negated.
};

}
