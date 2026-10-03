// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariables_fwd.hpp"

#include "impl/environment/EnvironmentPaths.hpp"
#include "impl/environment/EnvironmentVariableBackend.hpp"

#include "../path/Path.hpp"
#include "../text/String.hpp"

#include <optional>

namespace erbsland::system {

/// Provides portable access to process environment variables.
/// @seedoc{/reference/system/system_services}
/// @tested{EnvironmentVariablesTest EnvironmentPathsTest}
class EnvironmentVariables final {
public:
    /// Create an instance using the backend for the current platform.
    EnvironmentVariables();
    /// Create an instance using a custom backend.
    /// @param backend The custom backend, which must not be null.
    explicit EnvironmentVariables(impl::EnvironmentVariableBackendPtr backend);

    // defaults/deletions
    ~EnvironmentVariables() = default;
    EnvironmentVariables(const EnvironmentVariables &) = delete;
    EnvironmentVariables(EnvironmentVariables &&) = delete;
    auto operator=(const EnvironmentVariables &) -> EnvironmentVariables & = delete;
    auto operator=(EnvironmentVariables &&) -> EnvironmentVariables & = delete;

public: // variables
    /// Read an environment variable without throwing.
    /// @param name The variable name.
    /// @return The variable value, or `std::nullopt` if it does not exist or the lookup fails.
    [[nodiscard]] auto get(const text::String &name) const noexcept -> std::optional<text::String>;
    /// Read an environment variable without throwing, using a fallback value.
    /// @param name The variable name.
    /// @param defaultValue The value returned if the variable does not exist or the lookup fails.
    /// @return The variable value or the supplied fallback.
    [[nodiscard]] auto get(const text::String &name, text::String defaultValue) const noexcept -> text::String;
    /// Read an environment variable.
    /// @param name The variable name. Must not be empty or contain `=` or a null byte.
    /// @return The variable value, including an empty value if one is stored.
    /// @throws PlatformError If the native lookup fails.
    /// @throws err::ParseError If the name or value contains invalid environment-variable syntax.
    /// @throws err::NotFoundError If the variable is absent.
    [[nodiscard]] auto getOrThrow(const text::String &name) const -> text::String;
    /// Set an environment variable without throwing.
    /// @param name The variable name.
    /// @param value The new variable value.
    /// @return `true` on success, otherwise `false`.
    auto set(const text::String &name, const text::String &value) noexcept -> bool;
    /// Set an environment variable.
    /// @param name The variable name. Must not be empty or contain `=` or a null byte.
    /// @param value The new variable value. Must not contain a null byte.
    /// @throws PlatformError If the native operation fails.
    /// @throws err::ParseError If the name or value contains invalid environment-variable syntax.
    void setOrThrow(const text::String &name, const text::String &value);
    /// Remove an environment variable without throwing.
    /// Removing a variable that does not exist succeeds.
    /// @param name The variable name.
    /// @return `true` on success, otherwise `false`.
    auto remove(const text::String &name) noexcept -> bool;
    /// Remove an environment variable.
    /// Removing a variable that does not exist succeeds.
    /// @param name The variable name. Must not be empty or contain `=` or a null byte.
    /// @throws PlatformError If the native operation fails.
    /// @throws err::ParseError If the name or value contains invalid environment-variable syntax.
    void removeOrThrow(const text::String &name);

public: // PATH access
    /// Read valid, deduplicated PATH entries in search order without throwing.
    /// @return Normalized paths, or an empty list if PATH is absent or inaccessible.
    [[nodiscard]] auto paths() const noexcept -> path::PathList;
    /// Read valid, deduplicated PATH entries in search order.
    /// @return Normalized paths; absent PATH produces an empty list.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode the environment value.
    [[nodiscard]] auto pathsOrThrow() const -> path::PathList;
    /// Get native executable candidates without checking the filesystem or throwing.
    /// @param name A filename or absolute native path; Windows adds .exe when no suffix is present.
    /// @return Candidates in search order, or an empty list on failure.
    [[nodiscard]] auto executableCandidates(const text::String &name) const noexcept -> path::PathList;
    /// Get native executable candidates without checking the filesystem.
    /// @param name A filename or absolute native path; Windows adds .exe when no suffix is present.
    /// @return PATH candidates, or a single explicitly supplied absolute path.
    /// @throws err::ParameterFailure If the executable name is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    [[nodiscard]] auto executableCandidatesOrThrow(const text::String &name) const -> path::PathList;
    /// Move a path to the front of PATH without throwing.
    /// @param path The path to prepend.
    /// @return Whether the change succeeded.
    auto prependPath(const path::Path &path) noexcept -> bool;
    /// Move a path to the front of PATH.
    /// @param path The path to prepend.
    /// @throws err::ParameterFailure If a supplied path or index is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    void prependPathOrThrow(const path::Path &path);
    /// Append a path unless it is already present in PATH without throwing.
    /// @param path The path to append.
    /// @return Whether the change succeeded.
    auto appendPath(const path::Path &path) noexcept -> bool;
    /// Append a path unless it is already present in PATH.
    /// @param path The path to append.
    /// @throws err::ParameterFailure If a supplied path or index is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    void appendPathOrThrow(const path::Path &path);
    /// Insert a path at an index, removing an equivalent entry first without throwing.
    /// @param index Position in the deduplicated list before removal; the end is allowed.
    /// @param path The path to insert.
    /// @return Whether the change succeeded.
    auto insertPath(unit::ItemIndex index, const path::Path &path) noexcept -> bool;
    /// Insert a path at an index, removing an equivalent entry first.
    /// @param index Position in the deduplicated list before removal; the end is allowed.
    /// @param path The path to insert.
    /// @throws err::ParameterFailure If a supplied path or index is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    void insertPathOrThrow(unit::ItemIndex index, const path::Path &path);
    /// Remove lexically equivalent paths from PATH without throwing.
    /// @param path The path to remove.
    /// @return Whether the change succeeded.
    auto removePath(const path::Path &path) noexcept -> bool;
    /// Remove lexically equivalent paths from PATH.
    /// @param path The path to remove.
    /// @throws err::ParameterFailure If a supplied path or index is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    void removePathOrThrow(const path::Path &path);
    /// Remove a directory and its descendants from PATH without throwing.
    /// @param base The directory to remove.
    /// @return Whether the change succeeded.
    auto removePathsIn(const path::Path &base) noexcept -> bool;
    /// Remove a directory and its descendants from PATH.
    /// @param base The directory to remove.
    /// @throws err::ParameterFailure If a supplied path or index is invalid.
    /// @throws PlatformError If environment access fails.
    /// @throws err::ParseError If a backend cannot decode an environment value.
    void removePathsInOrThrow(const path::Path &base);

private:
    /// Read PATH with the backend's policy and placeholder resolver.
    [[nodiscard]] auto readPaths() const -> impl::EnvironmentPaths;
    /// Validate an environment-variable name.
    static void validateName(const text::String &name);
    /// Validate an environment-variable value.
    static void validateValue(const text::String &value);
    /// Test whether text contains a null character.
    [[nodiscard]] static auto containsNull(const text::String &value) noexcept -> bool;

private:
    impl::EnvironmentVariableBackendPtr _backend; ///< The platform or custom backend.
};

}
