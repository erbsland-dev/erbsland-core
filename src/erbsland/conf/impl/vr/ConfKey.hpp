// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringList.hpp"

#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

/// A key that consists of one or multiple elements.
class ConfKey {
public:
    /// Create a key with multiple elements.
    /// @param elements The elements of the key.
    explicit ConfKey(text::StringList elements) : _elements{std::move(elements)} {}
    /// Create a key with a single element.
    /// @param oneElement The single element of the key.
    explicit ConfKey(text::String oneElement) { _elements.append(std::move(oneElement)); }

    // defaults
    ConfKey() = default;
    ~ConfKey() = default;
    ConfKey(const ConfKey &) = default;
    ConfKey(ConfKey &&) noexcept = default;
    auto operator=(const ConfKey &) -> ConfKey & = default;
    auto operator=(ConfKey &&) noexcept -> ConfKey & = default;

public:
    /// Compare all elements of two keys.
    /// @param other The other key to compare.
    /// @param caseSensitivity The case sensitivity to use for comparison.
    /// @return True if all elements are equal, false otherwise.
    [[nodiscard]] auto isEqual(const ConfKey &other, text::CaseSensitivity caseSensitivity) const noexcept -> bool;
    /// Compare a single element of two keys.
    /// @param other The other key to compare.
    /// @param caseSensitivity The case sensitivity to use for comparison.
    /// @param index The element index to compare.
    /// If the index is out of bounds for any of the compared keys, an empty string is compared.
    /// @return True if the elements are equal, false otherwise.
    [[nodiscard]] auto isEqual(
        const ConfKey &other, text::CaseSensitivity caseSensitivity, std::size_t index) const noexcept -> bool;
    /// Access all elements of this key.
    [[nodiscard]] auto elements() const noexcept -> const text::StringList &;
    /// Access a single element of this key.
    /// @return The element at the given index or an empty string if the index is out of bounds.
    [[nodiscard]] auto element(std::size_t index) const noexcept -> text::String;
    /// Convert this key to a text representation.
    /// For the text representation, all elements of the key are joined with a comma character.
    [[nodiscard]] auto toText() const noexcept -> text::String;
    /// Get the size (number of elements) of this key.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Get a hash for this key.
    [[nodiscard]] auto hash(text::CaseSensitivity caseSensitivity) const noexcept -> std::size_t;
    /// Get the hash for a single element
    [[nodiscard]] static auto elementHash(const text::String &element, text::CaseSensitivity caseSensitivity) noexcept
        -> std::size_t;

private:
    text::StringList _elements; ///< The elements of this key.
};

}
