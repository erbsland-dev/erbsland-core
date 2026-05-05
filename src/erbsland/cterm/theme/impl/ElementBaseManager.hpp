// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Element.hpp"

#include <unordered_map>

namespace erbsland::cterm::theme::impl {

/// Stores user-defined theme element base relationships.
class ElementBaseManager final {
public:
    /// Register a user-defined element base.
    /// @param element The custom element to register.
    /// @param base The direct base element.
    /// @throws std::invalid_argument If `element` or `base` is invalid.
    /// @throws std::logic_error If `element` is built-in or already registered with a different base.
    void registerElement(Element element, Element base);
    /// Get the direct base element for an element.
    /// @param element The element to query.
    /// @return The direct base element, or `Element::None` if there is none.
    [[nodiscard]] auto baseElementFor(Element element) const -> Element;

private:
    /// Get the base for built-in elements.
    [[nodiscard]] static auto builtInBaseElementFor(Element element) noexcept -> Element;

private:
    std::unordered_map<Element, Element> _baseElements; ///< User-defined direct base elements.
};

}
