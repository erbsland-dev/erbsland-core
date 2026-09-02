// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfiguration.hpp"
#include "TlsConfigurationParser_fwd.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

/// One labeled TLS configuration parsed from an external representation.
/// @seedoc{/topics/cryptology/configuring_tls}
/// @tested{TlsConfigurationParserTest}
class TlsConfigurationEntry final {
    friend class TlsConfigurationParser;

private:
    /// Create one labeled TLS configuration entry.
    /// @param label The validated registry label.
    /// @param configuration The complete TLS configuration.
    TlsConfigurationEntry(text::String label, TlsConfiguration configuration) noexcept :
        _label{std::move(label)}, _configuration{std::move(configuration)} {}

public:
    // defaults
    ~TlsConfigurationEntry() = default;
    TlsConfigurationEntry(const TlsConfigurationEntry &) = default;
    TlsConfigurationEntry(TlsConfigurationEntry &&) noexcept = default;
    auto operator=(const TlsConfigurationEntry &) -> TlsConfigurationEntry & = default;
    auto operator=(TlsConfigurationEntry &&) noexcept -> TlsConfigurationEntry & = default;

public:
    /// Get the registry label.
    [[nodiscard]] auto label() const noexcept -> const text::String & { return _label; }
    /// Get the parsed TLS configuration.
    [[nodiscard]] auto configuration() const noexcept -> const TlsConfiguration & { return _configuration; }
    /// Move the parsed TLS configuration out of this entry.
    [[nodiscard]] auto takeConfiguration() noexcept -> TlsConfiguration { return std::move(_configuration); }

private:
    text::String _label;             ///< Validated registry label.
    TlsConfiguration _configuration; ///< Parsed TLS configuration.
};

}
