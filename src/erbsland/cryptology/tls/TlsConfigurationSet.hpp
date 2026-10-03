// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfigurationEntry.hpp"
#include "TlsSettings.hpp"

#include <vector>

namespace erbsland::cryptology {

/// Owned shared settings and named profiles published as one TLS registry update.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsConfigurationSet final {
public: // accessors
    /// Get shared settings.
    [[nodiscard]] auto settings() const noexcept -> const TlsSettings & { return _settings; }
    /// Replace shared settings.
    void setSettings(TlsSettings value) { _settings = std::move(value); }
    /// Get the named profiles.
    [[nodiscard]] auto entries() const noexcept -> const std::vector<TlsConfigurationEntry> & { return _entries; }
    /// Append a named profile; publication validates labels, duplicates, and roles.
    /// @param label Nonempty application label.
    /// @param configuration The supplied role parts.
    void add(text::String label, TlsConfiguration configuration) {
        _entries.emplace_back(std::move(label), std::move(configuration));
    }

private:
    TlsSettings _settings;                       ///< Shared settings for this generation.
    std::vector<TlsConfigurationEntry> _entries; ///< Owned named profiles.
};

}
