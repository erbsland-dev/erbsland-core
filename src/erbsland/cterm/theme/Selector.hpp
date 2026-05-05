// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Element.hpp"
#include "Part.hpp"
#include "States.hpp"
#include "Tags.hpp"

#include "../impl/HashHelper.hpp"

#include <cstdint>
#include <functional>

namespace erbsland::cterm::theme {

/// A selector for one theme property sheet.
class Selector final {
public:
    /// Create an empty selector.
    constexpr Selector() noexcept = default;
    /// Create a selector for an element and optional part.
    /// @param element The selected element.
    /// @param part The selected part.
    constexpr explicit Selector(const Element element, const Part part = Part::None) noexcept :
        _element{element}, _part{part} {}

public: // operators
    /// Compare two selectors.
    auto operator==(const Selector &other) const noexcept -> bool = default;
    /// Compare two selectors.
    auto operator!=(const Selector &other) const noexcept -> bool = default;

public:
    /// Get the selected element.
    [[nodiscard]] constexpr auto element() const noexcept -> Element { return _element; }
    /// Get the selected part.
    [[nodiscard]] constexpr auto part() const noexcept -> Part { return _part; }
    /// Get the required states.
    [[nodiscard]] constexpr auto states() const noexcept -> States { return _states; }
    /// Get the required tags.
    [[nodiscard]] constexpr auto tags() const noexcept -> Tags { return _tags; }
    /// Create a copy with a different element.
    [[nodiscard]] constexpr auto withElement(const Element element) const noexcept -> Selector {
        auto result = *this;
        result._element = element;
        return result;
    }
    /// Create a copy with a different part.
    [[nodiscard]] constexpr auto withPart(const Part part) const noexcept -> Selector {
        auto result = *this;
        result._part = part;
        return result;
    }
    /// Create a copy with additional state flags.
    [[nodiscard]] auto withState(const State state) const noexcept -> Selector {
        auto result = *this;
        result._states.add(state);
        return result;
    }
    /// Create a copy with a state set.
    [[nodiscard]] constexpr auto withStates(const States states) const noexcept -> Selector {
        auto result = *this;
        result._states = states;
        return result;
    }
    /// Create a copy with an additional tag.
    [[nodiscard]] constexpr auto withTag(const Tag tag) const noexcept -> Selector {
        auto result = *this;
        result._tags.add(tag);
        return result;
    }
    /// Create a copy with a tag set.
    [[nodiscard]] constexpr auto withTags(const Tags tags) const noexcept -> Selector {
        auto result = *this;
        result._tags = tags;
        return result;
    }
    /// Add one required state to this selector.
    auto requireState(State state) noexcept -> Selector &;
    /// Add required states to this selector.
    auto requireStates(States states) noexcept -> Selector &;
    /// Add one required tag to this selector.
    auto requireTag(Tag tag) noexcept -> Selector &;
    /// Add required tags to this selector.
    auto requireTags(Tags tags) noexcept -> Selector &;
    /// Test if this selector matches the supplied state and tags.
    [[nodiscard]] auto matches(States state, Tags tags) const noexcept -> bool;
    /// Get the selector specificity.
    [[nodiscard]] auto specificity() const noexcept -> int;
    /// Get a stable hash for this selector.
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

private:
    Element _element; ///< The selected element.
    Part _part;       ///< The selected part.
    States _states;   ///< The required states.
    Tags _tags;       ///< The required tags.
};

}

template <>
struct std::hash<erbsland::cterm::theme::Selector> {
    auto operator()(const erbsland::cterm::theme::Selector &selector) const noexcept -> std::size_t {
        return selector.hash();
    }
};
