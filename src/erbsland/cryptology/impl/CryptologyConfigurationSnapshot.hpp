// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../configuration/CryptologyConfiguration_fwd.hpp"
#include "../CryptographicStatus.hpp"
#include "../HashAlgorithm.hpp"
#include "../symmetric/SymmetricEncryptionType.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl {

/// An immutable, internally consistent view of the cryptology configuration.
/// @tested{CryptologyConfigurationTest}
class CryptologyConfigurationSnapshot final {
    friend class erbsland::cryptology::CryptologyConfiguration;

public:
    // defaults
    CryptologyConfigurationSnapshot() = default;
    ~CryptologyConfigurationSnapshot() = default;
    CryptologyConfigurationSnapshot(const CryptologyConfigurationSnapshot &) = default;
    CryptologyConfigurationSnapshot(CryptologyConfigurationSnapshot &&) noexcept = default;
    auto operator=(const CryptologyConfigurationSnapshot &) -> CryptologyConfigurationSnapshot & = default;
    auto operator=(CryptologyConfigurationSnapshot &&) noexcept -> CryptologyConfigurationSnapshot & = default;

public: // accessors
    /// Test whether architecture-specific cryptographic backends may be selected.
    [[nodiscard]] auto hardwareAccelerationEnabled() const noexcept -> bool { return _hardwareAccelerationEnabled; }
    /// Get the administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm to inspect.
    /// @return The configured limit, or no value if no limit is configured or the value is invalid.
    [[nodiscard]] auto maximumStatus(HashAlgorithm algorithm) const noexcept -> std::optional<CryptographicStatus>;
    /// Get the administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type to inspect.
    /// @return The configured limit, or no value if no limit is configured or the value is invalid.
    [[nodiscard]] auto maximumStatus(SymmetricEncryptionType type) const noexcept -> std::optional<CryptographicStatus>;

private:
    static constexpr uint8_t cNoStatusLimit{0xffU};                ///< Marker for no status limit.
    static constexpr std::size_t cHashAlgorithmCount{8U};          ///< Number of hash values.
    static constexpr std::size_t cSymmetricTypeStorageCount{6U};   ///< Symmetric values including `None`.

    bool _hardwareAccelerationEnabled{true};                       ///< Whether acceleration may be used.
    std::array<uint8_t, cHashAlgorithmCount> _hashMaximumStatus{}; ///< Per-hash limits.
    /// Per-symmetric-encryption-type limits.
    std::array<uint8_t, cSymmetricTypeStorageCount> _symmetricMaximumStatus{};
};

}
