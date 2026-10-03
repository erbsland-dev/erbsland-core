// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../path/Path.hpp"

#include <optional>

namespace erbsland::cryptology {

/// Shared settings captured with an application TLS configuration set.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsSettings final {
public: // accessors
    /// Get the shared trust selection, or no value for role defaults.
    [[nodiscard]] auto systemTrust() const noexcept -> std::optional<bool> { return _systemTrust; }
    /// Set the shared trust selection; no value restores role defaults.
    void setSystemTrust(std::optional<bool> value) noexcept { _systemTrust = value; }
    /// Test whether native trust evaluation may retrieve certificates and revocation information.
    [[nodiscard]] auto allowsTrustNetwork() const noexcept -> bool { return _trustAllowNetwork; }
    /// Set native trust retrieval permission.
    void setTrustAllowNetwork(bool value) noexcept { _trustAllowNetwork = value; }
    /// Get the Linux system trust bundle override.
    [[nodiscard]] auto trustBundlePath() const noexcept -> const path::Path & { return _trustBundlePath; }
    /// Set the Linux system trust bundle override.
    void setTrustBundlePath(path::Path value) { _trustBundlePath = std::move(value); }
    /// Test whether loading and using native identities may display authorization dialogs.
    [[nodiscard]] auto allowsIdentityInteraction() const noexcept -> bool { return _identityAllowInteraction; }
    /// Set permission captured when native identities are loaded.
    void setIdentityAllowInteraction(bool value) noexcept { _identityAllowInteraction = value; }

private:
    std::optional<bool> _systemTrust; ///< Shared selection, absent for role defaults.
    bool _trustAllowNetwork{true};    ///< Native trust retrieval permission.
    path::Path _trustBundlePath;      ///< Optional Linux bundle override.
    bool _identityAllowInteraction{}; ///< Native identity authorization permission.
};

}
