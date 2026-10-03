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
    [[nodiscard]] auto pathSeparator() const noexcept -> text::Char override;
    [[nodiscard]] auto compare(const text::String &left, const text::String &right) const
        -> std::strong_ordering override;
    [[nodiscard]] auto compare(const text::U16String &left, const text::U16String &right) const
        -> std::strong_ordering override;
    [[nodiscard]] auto parsePath(const text::String &value) const -> path::Path override;
    [[nodiscard]] auto pathText(const path::Path &path) const -> text::String override;
    [[nodiscard]] auto expandPath(const text::String &value, const Lookup &lookup) const
        -> std::optional<text::String> override;
    [[nodiscard]] auto isLiteralPath(const text::String &value) const -> bool override;
    [[nodiscard]] auto executableName(const path::Path &path) const -> text::String override;
};

}
