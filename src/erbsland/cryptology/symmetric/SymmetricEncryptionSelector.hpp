// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricEncryptionRequirements.hpp"
#include "SymmetricEncryptionSelector_fwd.hpp"
#include "SymmetricEncryptionType.hpp"

#include "../configuration/CryptologyConfiguration.hpp"
#include "../CryptographicStatus.hpp"
#include "../impl/CryptologyConfigurationSnapshot.hpp"

#include "../../util/List_fwd.hpp"

#include <optional>

namespace erbsland::cryptology {

/// Select symmetric encryption constructions using requirements and the live application-wide cryptology policy.
/// Each operation uses one coherent configuration snapshot. Explicit encryptor and decryptor construction is
/// unaffected.
/// @seedoc{/reference/cryptology/symmetric_encryption}
/// @tested{SymmetricEncryptionTypeTest CryptologyConfigurationTest}
class SymmetricEncryptionSelector final {
public:
    /// Create a selector with the default requirements.
    SymmetricEncryptionSelector() = default;
    /// Create a selector with explicit requirements.
    /// @param requirements The requirements used by `matches()`, `matching()`, and `recommended()`.
    explicit SymmetricEncryptionSelector(SymmetricEncryptionRequirements requirements) noexcept :
        _requirements{requirements} {}

    // defaults
    ~SymmetricEncryptionSelector() = default;
    SymmetricEncryptionSelector(const SymmetricEncryptionSelector &) = default;
    SymmetricEncryptionSelector(SymmetricEncryptionSelector &&) noexcept = default;
    auto operator=(const SymmetricEncryptionSelector &) -> SymmetricEncryptionSelector & = default;
    auto operator=(SymmetricEncryptionSelector &&) noexcept -> SymmetricEncryptionSelector & = default;

public: // tests
    /// Test whether an encryption type satisfies this selector's requirements.
    [[nodiscard]] auto matches(SymmetricEncryptionType type) const -> bool;

public: // accessors
    /// Get the selector requirements.
    [[nodiscard]] auto requirements() const noexcept -> const SymmetricEncryptionRequirements & {
        return _requirements;
    }
    /// Get the encryption type's effective status under the live application-wide policy.
    [[nodiscard]] auto status(SymmetricEncryptionType type) const -> CryptographicStatus;

public: // selection
    /// Get all currently acceptable encryption types in stable preference order.
    [[nodiscard]] auto allAccepted() const -> util::List<SymmetricEncryptionType>;
    /// Get all encryption types that satisfy this selector's requirements.
    [[nodiscard]] auto matching() const -> util::List<SymmetricEncryptionType>;
    /// Get the first preferred encryption type satisfying this selector's requirements.
    [[nodiscard]] auto recommended() const -> std::optional<SymmetricEncryptionType>;

private:
    /// Get the status built into the library for an encryption type.
    [[nodiscard]] static auto libraryStatus(SymmetricEncryptionType type) noexcept -> CryptographicStatus;
    /// Apply the current configuration to a library status.
    [[nodiscard]] static auto effectiveStatus(
        SymmetricEncryptionType type, const impl::CryptologyConfigurationSnapshot &configuration) noexcept
        -> CryptographicStatus;
    /// Test an encryption type against the requirements and configuration snapshot.
    [[nodiscard]] auto matches(
        SymmetricEncryptionType type, const impl::CryptologyConfigurationSnapshot &configuration) const noexcept
        -> bool;

private:
    SymmetricEncryptionRequirements _requirements; ///< Requirements used for matching and recommendation.
};

}
