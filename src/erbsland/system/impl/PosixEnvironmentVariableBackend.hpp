// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariableBackend.hpp"

namespace erbsland::system::impl {

/// POSIX implementation for process environment variables.
/// @tested{EnvironmentVariablesTest}
class PosixEnvironmentVariableBackend final : public EnvironmentVariableBackend {
public:
    /// Create the POSIX environment-variable backend.
    PosixEnvironmentVariableBackend() = default;

public: // implement EnvironmentVariableBackend
    /// @copydoc EnvironmentVariableBackend::get
    [[nodiscard]] auto get(const text::String &name) const -> std::optional<text::String> override;
    /// @copydoc EnvironmentVariableBackend::set
    void set(const text::String &name, const text::String &value) override;
    /// @copydoc EnvironmentVariableBackend::remove
    void remove(const text::String &name) override;
};

}
