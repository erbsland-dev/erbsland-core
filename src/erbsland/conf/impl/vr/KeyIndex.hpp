// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"
#include "KeyIndex_fwd.hpp"
#include "KeyIndexData_fwd.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../Name.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

/// A key index is a collection of keys to validate unique values and references.
/// Keys can consist of a single element or multiple elements.
/// An index can be case-sensitive or case-insensitive.
class KeyIndex {
public:
    /// Create a new key index.
    /// @param name The optional name.
    /// @param caseSensitivity The case sensitivity of the keys.
    /// @param elementCount The number of key elements for every key.
    explicit KeyIndex(Name name, text::CaseSensitivity caseSensitivity, std::size_t elementCount);

    // defaults
    ~KeyIndex();

public:
    /// Access the name of this key index.
    [[nodiscard]] auto name() const noexcept -> const Name & { return _name; }

    /// Get the case sensitivity of this key index.
    [[nodiscard]] auto caseSensitivity() const noexcept -> text::CaseSensitivity { return _caseSensitivity; }

    /// Try to add a key to this index.
    /// @param key The key to add.
    /// @return True if the key was added, false if it was already present.
    [[nodiscard]] auto tryAddKey(const ConfKey &key) -> bool;

    /// Test if a key is present in this index.
    /// @param keyString The full key to test.
    /// @return True if the key is present, false otherwise.
    [[nodiscard]] auto hasKey(const text::String &keyString) const noexcept -> bool;

    /// Test if a key is present in this index.
    /// @param key The full key to test.
    /// @return True if the key is present, false otherwise.
    [[nodiscard]] auto hasKey(const ConfKey &key) const noexcept -> bool;

    /// Test a partial key in this index.
    /// @param keyString The key element to test.
    /// @param index The element index to test.
    /// @return True if the key element is present, false otherwise.
    [[nodiscard]] auto hasKey(const text::String &keyString, std::size_t index) const noexcept -> bool;

private:
    Name _name;                ///< The name if this index for references.
    text::CaseSensitivity _caseSensitivity;
    std::size_t _elementCount; ///< The number of key elements for every key.
    KeyIndexDataPtr _data;     ///< The data instance to store the keys.
};

}
