// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicStatus.hpp"
#include "HashAlgorithm.hpp"
#include "HashRequirements.hpp"
#include "HashSelector_fwd.hpp"

#include "configuration/CryptologyConfiguration.hpp"
#include "impl/CryptologyConfigurationSnapshot.hpp"

#include "../util/List_fwd.hpp"

#include <optional>

namespace erbsland::cryptology {

/// Select hash algorithms using requirements and the live application-wide cryptology policy.
/// Each operation uses one coherent configuration snapshot. Explicit `Hasher` construction is unaffected by policy.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{HashAlgorithmTest CryptologyConfigurationTest}
class HashSelector final {
public:
    /// Create a selector with the default requirements.
    HashSelector() = default;
    /// Create a selector with explicit requirements.
    /// @param requirements The requirements used by `matches()`, `matching()`, and `recommended()`.
    explicit HashSelector(HashRequirements requirements) noexcept : _requirements{requirements} {}

    // defaults
    ~HashSelector() = default;
    HashSelector(const HashSelector &) = default;
    HashSelector(HashSelector &&) noexcept = default;
    auto operator=(const HashSelector &) -> HashSelector & = default;
    auto operator=(HashSelector &&) noexcept -> HashSelector & = default;

public: // tests
    /// Test whether an algorithm is acceptable with at least standard security.
    [[nodiscard]] auto isSafe(HashAlgorithm algorithm) const -> bool;
    /// Test whether an algorithm satisfies this selector's requirements.
    [[nodiscard]] auto matches(HashAlgorithm algorithm) const -> bool;

public: // accessors
    /// Get the selector requirements.
    [[nodiscard]] auto requirements() const noexcept -> const HashRequirements & { return _requirements; }
    /// Get the algorithm's effective status under the live application-wide policy.
    [[nodiscard]] auto status(HashAlgorithm algorithm) const -> CryptographicStatus;

public: // selection
    /// Get all algorithms currently acceptable with at least standard security.
    [[nodiscard]] auto allAccepted() const -> util::List<HashAlgorithm>;
    /// Get all algorithms that satisfy this selector's requirements.
    [[nodiscard]] auto matching() const -> util::List<HashAlgorithm>;
    /// Get the preferred algorithm satisfying this selector's requirements.
    /// Selection prefers throughput, then security, then stable declaration order.
    [[nodiscard]] auto recommended() const -> std::optional<HashAlgorithm>;

private:
    /// Test whether the raw value identifies one of the declared hash algorithms.
    [[nodiscard]] static auto isValid(HashAlgorithm algorithm) noexcept -> bool;
    /// Get the status built into the library for an algorithm.
    [[nodiscard]] static auto libraryStatus(HashAlgorithm algorithm) noexcept -> CryptographicStatus;
    /// Apply the current configuration to a library status.
    [[nodiscard]] static auto effectiveStatus(
        HashAlgorithm algorithm, const impl::CryptologyConfigurationSnapshot &configuration) noexcept
        -> CryptographicStatus;
    /// Test an algorithm against the requirements and configuration snapshot.
    [[nodiscard]] auto matches(
        HashAlgorithm algorithm, const impl::CryptologyConfigurationSnapshot &configuration) const noexcept -> bool;

private:
    HashRequirements _requirements; ///< Requirements used for matching and recommendation.
};

}
