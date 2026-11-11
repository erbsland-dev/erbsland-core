// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyDefinition_fwd.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../Location.hpp"
#include "../../Name.hpp"
#include "../../NamePath.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// A key definition and its value paths resolved during rule finalization.
/// @tested{VrKeysAndReferencesTest VrLiteralNamesTest VrLiteralPathsTest}
class KeyDefinition {
public:
    /// Symbolic value paths relative to the section owning this definition.
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
    /// Create an unresolved key definition.
    /// @param name The optional literal index name; document escapes are not decoded.
    /// @param keys Symbolic value paths relative to the owning rule.
    /// @param caseSensitivity The key comparison sensitivity.
    /// @param location The original definition location.
    /// @return A new definition without resolved paths.
    [[nodiscard]] static auto create(Name name, Keys keys, text::CaseSensitivity caseSensitivity, Location location)
        -> KeyDefinitionPtr;

public:
    /// Get the optional key name.
    [[nodiscard]] auto name() const noexcept -> const Name & { return _name; }
    /// Get the original symbolic paths, preserved for definition diagnostics.
    [[nodiscard]] auto keys() const noexcept -> const Keys & { return _keys; }
    /// Get the key comparison case sensitivity.
    [[nodiscard]] auto caseSensitivity() const noexcept -> text::CaseSensitivity { return _caseSensitivity; }
    /// Get the source location of the definition.
    [[nodiscard]] auto location() const noexcept -> const Location & { return _location; }

public: // resolved paths
    /// Get the list path relative to the owning section after successful finalization.
    [[nodiscard]] auto listPath() const noexcept -> const NamePath & { return _listPath; }
    /// Get the paths relative to each list entry after successful finalization, in key order.
    [[nodiscard]] auto entryValuePaths() const noexcept -> const NamePathList & { return _entryValuePaths; }
    /// Publish paths after validating every component of this definition.
    /// @param listPath The non-empty path to the shared section list, relative to the owning section.
    /// @param entryValuePaths One non-empty entry-relative path per original key.
    void setResolvedPaths(NamePath listPath, NamePathList entryValuePaths) {
        _listPath = std::move(listPath);
        _entryValuePaths = std::move(entryValuePaths);
    }

private:
    NamePath _listPath;                          ///< Finalized list path; empty until definition validation succeeds.
    NamePathList _entryValuePaths;               ///< Finalized per-entry value paths without control placeholders.
    Name _name;                                  ///< The optional name. Empty if undefined.
    Keys _keys;                                  ///< The key path(s) to the value(s).
    text::CaseSensitivity _caseSensitivity{
        text::CaseSensitivity::CaseInsensitive}; ///< If the index shall be case-sensitive.
    Location _location;                          ///< Original key definition location, retained after path resolution.
};

}
