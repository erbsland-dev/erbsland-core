// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariables_fwd.hpp"

#include "impl/EnvironmentVariableBackend.hpp"

#include "../text/String.hpp"

#include <optional>

namespace erbsland::system {

/// Provides portable access to process environment variables.
/// @seedoc{/reference/system/environment_variables}
/// @tested{EnvironmentVariablesTest}
class EnvironmentVariables final {
public:
    /// Create an instance using the backend for the current platform.
    EnvironmentVariables();
    /// Create an instance using a custom backend.
    /// @param backend The custom backend, which must not be null.
    /// @throws err::ParameterError If `backend` is null.
    explicit EnvironmentVariables(impl::EnvironmentVariableBackendPtr backend);

    // defaults/deletions
    ~EnvironmentVariables() = default;
    EnvironmentVariables(const EnvironmentVariables &) = delete;
    EnvironmentVariables(EnvironmentVariables &&) = delete;
    auto operator=(const EnvironmentVariables &) -> EnvironmentVariables & = delete;
    auto operator=(EnvironmentVariables &&) -> EnvironmentVariables & = delete;

public:
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
    /// @param name The variable name.
    /// @return The variable value, including an empty value if one is stored.
    /// @throws err::ParameterError If `name` is invalid.
    /// @throws PlatformError If the variable does not exist or the native lookup fails.
    [[nodiscard]] auto getOrThrow(const text::String &name) const -> text::String;
    /// Set an environment variable without throwing.
    /// @param name The variable name.
    /// @param value The new variable value.
    /// @return `true` on success, otherwise `false`.
    auto set(const text::String &name, const text::String &value) noexcept -> bool;
    /// Set an environment variable.
    /// @param name The variable name.
    /// @param value The new variable value.
    /// @throws err::ParameterError If `name` or `value` is invalid.
    /// @throws PlatformError If the native operation fails.
    void setOrThrow(const text::String &name, const text::String &value);
    /// Remove an environment variable without throwing.
    /// Removing a variable that does not exist succeeds.
    /// @param name The variable name.
    /// @return `true` on success, otherwise `false`.
    auto remove(const text::String &name) noexcept -> bool;
    /// Remove an environment variable.
    /// Removing a variable that does not exist succeeds.
    /// @param name The variable name.
    /// @throws err::ParameterError If `name` is invalid.
    /// @throws PlatformError If the native operation fails.
    void removeOrThrow(const text::String &name);

private:
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
