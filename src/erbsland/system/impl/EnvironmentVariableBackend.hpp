// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariableBackend_fwd.hpp"

#include "../../text/String.hpp"

#include <optional>

namespace erbsland::system::impl {

/// Backend interface for process environment variables.
/// @tested{EnvironmentVariablesTest}
class EnvironmentVariableBackend {
public:
    // defaults
    virtual ~EnvironmentVariableBackend() = default;

public:
    /// Read an environment variable.
    /// @param name The validated variable name.
    /// @return The variable value, or `std::nullopt` if the variable does not exist.
    /// @throws PlatformError If the native lookup fails.
    [[nodiscard]] virtual auto get(const text::String &name) const -> std::optional<text::String> = 0;
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
};

/// Create the environment-variable backend for the current platform.
/// @return The created platform backend.
/// @tested{EnvironmentVariablesTest}
[[nodiscard]] auto createEnvironmentVariableBackend() -> EnvironmentVariableBackendPtr;

}
