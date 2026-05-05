// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Properties.hpp"
#include "PropertySheet.hpp"
#include "Selector.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace erbsland::cterm::theme {

namespace impl {
class ElementBaseManager;
}

class Theme;
using ThemePtr = std::shared_ptr<Theme>;
using ThemeConstPtr = std::shared_ptr<const Theme>;

/// Immutable theme built from hierarchical property sheets.
class Theme final {
public:
    /// One authored property sheet definition.
    struct Definition final {
        Properties properties; ///< The authored values.
        uint64_t order{};      ///< The insertion order.
    };
    /// An optional definition.
    using OptionalDefinition = std::optional<std::reference_wrapper<const Definition>>;
    /// The authored property sheet definitions.
    using Definitions = std::unordered_map<Selector, Definition>;

public:
    /// Create a theme from authored property sheet definitions.
    /// @param definitions The authored property sheet definitions.
    /// @param registeredTagCount The number of registered tags.
    explicit Theme(Definitions definitions, uint8_t registeredTagCount = 0);

public:
    /// Resolve an effective property sheet for a selector.
    /// @param selector The selector to resolve.
    [[nodiscard]] auto propertySheet(Selector selector) const -> PropertySheetConstPtr;
    /// Get the number of registered tags.
    [[nodiscard]] auto registeredTagCount() const noexcept -> uint8_t { return _registeredTagCount; }
    /// Access the authored definitions.
    [[nodiscard]] auto definitions() const noexcept -> const Definitions & { return _definitions; }

public:
    /// Register a user-defined element base relationship.
    /// @param element The user-defined element.
    /// @param base The direct base element.
    static void registerElement(Element element, Element base);
    /// Access the predefined dark theme.
    [[nodiscard]] static auto dark() noexcept -> const ThemeConstPtr &;
    /// Access the predefined light theme.
    [[nodiscard]] static auto light() noexcept -> const ThemeConstPtr &;
    /// Access the predefined monochrome theme.
    [[nodiscard]] static auto monochrome() noexcept -> const ThemeConstPtr &;
    /// Access the predefined zero theme.
    /// The zero theme does not define any elements or styles.
    /// Use this theme for your unit tests or when you want to start from a clean slate.
    [[nodiscard]] static auto zero() noexcept -> const ThemeConstPtr &;

private:
    /// Normalize empty selectors to the root selector.
    [[nodiscard]] static auto normalized(Selector selector) noexcept -> Selector;
    /// Find an authored definition.
    [[nodiscard]] auto definitionFor(const Selector &selector) const noexcept -> OptionalDefinition;
    /// Find the selector for the nearest base sheet.
    [[nodiscard]] auto baseSelectorFor(const Selector &selector) const noexcept -> Selector;
    /// Find the best authored tag subset.
    [[nodiscard]] auto bestTagSubset(const Selector &selector) const noexcept -> std::optional<Selector>;
    /// Find the best authored state subset.
    [[nodiscard]] auto bestStateSubset(const Selector &selector) const noexcept -> std::optional<Selector>;
    /// Access the global element base manager.
    [[nodiscard]] static auto elementBaseManager() noexcept -> impl::ElementBaseManager &;

private:
    Definitions _definitions;                                           ///< Authored property sheets.
    uint8_t _registeredTagCount{};                                      ///< Number of registered tag bits.
    mutable std::unordered_map<Selector, PropertySheetConstPtr> _cache; ///< Effective sheet cache.
};

}
