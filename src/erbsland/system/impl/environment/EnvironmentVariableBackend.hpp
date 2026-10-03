// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariableBackend_fwd.hpp"

#include "../../../path/Path.hpp"
#include "../../../text/String.hpp"
#include "../../../text/u16/U16String.hpp"

#include <compare>
#include <functional>
#include <optional>

namespace erbsland::system::impl {

/// Backend interface for process environment variables.
/// @tested{EnvironmentVariablesTest EnvironmentPathsTest EnvironmentChangesTest}
class EnvironmentVariableBackend {
public:
    /// Canonical name of the executable-search environment variable.
    constexpr static auto cPathVariable = text::StringLiteral{"PATH"};
    /// Lookup of a placeholder in the environment currently being processed.
    using Lookup = std::function<std::optional<text::String>(const text::String &)>;

public:
    // defaults
    virtual ~EnvironmentVariableBackend() = default;

public: // environment access
    /// Read an environment variable.
    /// @param name The validated variable name.
    /// @return The variable value, or `std::nullopt` if the variable does not exist.
    /// @throws PlatformError If the native lookup fails.
    [[nodiscard]] virtual auto get(const text::String &name) const -> std::optional<text::String> = 0;
    /// Read a PATH placeholder value without replacing malformed native text.
    /// @param name The variable referenced by the placeholder.
    /// @return The value, or no value when absent or malformed.
    /// @throws PlatformError If the lookup fails.
    [[nodiscard]] virtual auto getPathVariable(const text::String &name) const -> std::optional<text::String>;
    /// Read PATH without replacing malformed native entries with valid-looking text.
    /// @return The native value, or no value when PATH is absent.
    /// @throws PlatformError If the lookup fails.
    [[nodiscard]] virtual auto getPath() const -> std::optional<text::String>;
    /// Set an environment variable.
    /// @param name The validated variable name.
    /// @param value The validated variable value.
    /// @throws PlatformError If the native operation fails.
    virtual void set(const text::String &name, const text::String &value) = 0;
    /// Remove an environment variable.
    /// Removing a variable that does not exist succeeds.
    /// @param name The validated variable name.
    /// @throws PlatformError If the native operation fails.
    virtual void remove(const text::String &name) = 0;

public: // platform policy
    /// Get the PATH entry separator.
    [[nodiscard]] virtual auto pathSeparator() const noexcept -> text::Char;
    /// Compare UTF-8 environment names or normalized path components.
    /// @param left First name or component.
    /// @param right Second name or component.
    /// @return Native case-sensitive or case-insensitive ordering.
    [[nodiscard]] virtual auto compare(const text::String &left, const text::String &right) const
        -> std::strong_ordering;
    /// Compare native UTF-16 names without replacing malformed code units.
    /// @param left First name or component.
    /// @param right Second name or component.
    /// @return Native ordering.
    [[nodiscard]] virtual auto compare(const text::U16String &left, const text::U16String &right) const
        -> std::strong_ordering;
    /// Parse native path text without resolving it against the filesystem.
    /// @param value A nonempty native path spelling.
    /// @return The parsed path, or empty for malformed input.
    [[nodiscard]] virtual auto parsePath(const text::String &value) const -> path::Path;
    /// Encode a path using native separators.
    /// @param path The path to encode.
    /// @return Native path text.
    [[nodiscard]] virtual auto pathText(const path::Path &path) const -> text::String;
    /// Resolve native quoting and placeholders in one validated UTF-8 PATH entry.
    /// @param value Original entry text, without the PATH separator.
    /// @param lookup Lookup of placeholders in the processed environment.
    /// @return Expanded path text, or no value for malformed or unresolved entries.
    [[nodiscard]] virtual auto expandPath(const text::String &value, const Lookup &lookup) const
        -> std::optional<text::String>;
    /// Test whether explicit path text avoids ambiguous native quoting or expansion syntax.
    /// @param value Native spelling of a caller-supplied path.
    /// @return Whether it can be stored literally in PATH.
    [[nodiscard]] virtual auto isLiteralPath(const text::String &value) const -> bool;
    /// Add the native executable suffix when required.
    /// @param path Parsed executable name or absolute path.
    /// @return The executable filename to search for.
    [[nodiscard]] virtual auto executableName(const path::Path &path) const -> text::String;

public: // shared path operations
    /// Parse and normalize a path while retaining relative paths and treating empty entries as '.'.
    /// @param value Native path text.
    /// @return A normalized path, or empty if malformed.
    [[nodiscard]] auto pathKey(const text::String &value) const -> path::Path;
    /// Compare complete normalized paths using this backend's component comparison.
    /// @param left First path.
    /// @param right Second path.
    /// @return Whether both paths are equivalent.
    [[nodiscard]] auto samePath(const path::Path &left, const path::Path &right) const -> bool;
    /// Test whether a normalized path is equal to or contained in a base directory.
    /// @param path Candidate path.
    /// @param base Base directory.
    /// @return Whether the complete base components match.
    [[nodiscard]] auto inBase(const path::Path &path, const path::Path &base) const -> bool;

public: // factories
    /// Access the immutable native backend shared by deferred environment transformations.
    /// Custom storage backends inherit this backend's platform policy unless they override it.
    [[nodiscard]] static auto native() -> const EnvironmentVariableBackend &;
};

/// Create the environment-variable backend for the current platform.
/// @return The created platform backend.
/// @tested{EnvironmentVariablesTest EnvironmentPathsTest EnvironmentChangesTest}
[[nodiscard]] auto createEnvironmentVariableBackend() -> EnvironmentVariableBackendPtr;

}
