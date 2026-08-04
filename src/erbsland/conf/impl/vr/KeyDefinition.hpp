// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyDefinition_fwd.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../Location.hpp"
#include "../../Name.hpp"
#include "../../NamePath.hpp"

namespace erbsland::conf::impl {

/// Defines a key entry, based on a `vr_key` definition.
class KeyDefinition {
public:
    using Keys = NamePathList;

public:
    /// Create a definition for a configured key entry.
    /// @param name The optional key name.
    /// @param keys The associated key paths.
    /// @param caseSensitivity The key comparison sensitivity.
    /// @param location The source location of the definition.
    KeyDefinition(Name name, Keys keys, text::CaseSensitivity caseSensitivity, Location location) noexcept;

    // defaults
    ~KeyDefinition() = default;

public:
    /// Create a shared key definition.
    [[nodiscard]] static auto create(Name name, Keys keys, text::CaseSensitivity caseSensitivity, Location location)
        -> KeyDefinitionPtr;

public:
    /// Get the optional key name.
    [[nodiscard]] auto name() const noexcept -> const Name & { return _name; }
    /// Get the value key paths.
    [[nodiscard]] auto keys() const noexcept -> const Keys & { return _keys; }
    /// Get the key comparison case sensitivity.
    [[nodiscard]] auto caseSensitivity() const noexcept -> text::CaseSensitivity { return _caseSensitivity; }
    /// Get the source location of the definition.
    [[nodiscard]] auto location() const noexcept -> const Location & { return _location; }

public:
    Name _name;                                  ///< The optional name. Empty if undefined.
    Keys _keys;                                  ///< The key path(s) to the value(s).
    text::CaseSensitivity _caseSensitivity{
        text::CaseSensitivity::CaseInsensitive}; ///< If the index shall be case-sensitive.
    Location _location;
};

}
