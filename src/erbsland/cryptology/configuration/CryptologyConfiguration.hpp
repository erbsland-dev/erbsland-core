// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CryptographicStatus.hpp"
#include "../CryptologyError.hpp"
#include "../HashAlgorithm.hpp"
#include "../HashSelector_fwd.hpp"
#include "../impl/CryptologyConfigurationSnapshot.hpp"
#include "../impl/protected_data/ProtectedDataAccess_fwd.hpp"
#include "../impl/protected_data/ProtectedDataProvider_fwd.hpp"
#include "../impl/TlsConfigurationLabel.hpp"
#include "../protected_data/ProtectedByteBlock_fwd.hpp"
#include "../protected_data/ProtectedDataMode.hpp"
#include "../symmetric/SymmetricEncryptionSelector_fwd.hpp"
#include "../symmetric/SymmetricEncryptionType.hpp"
#include "../tls/TlsConfiguration.hpp"
#include "../tls/TlsConfigurationResolution.hpp"
#include "../tls/TlsConfigurationSet.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/StringHashMap.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/ByteCount.hpp"

#include <mutex>
#include <optional>

namespace erbsland::cryptology {

/// Application-wide administrative limits for cryptographic algorithm selection and backend acceleration.
/// Status limits can only reduce the effective library policy. They never prevent explicit primitive use.
/// Call `core::application().cryptologyConfiguration()` to access the shared instance.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{CryptologyConfigurationTest}
class CryptologyConfiguration final {
    friend class HashSelector;
    friend class SymmetricEncryptionSelector;
    friend class impl::ApplicationProtectedDataAccess;

public:
    /// Create the default configuration.
    CryptologyConfiguration();

    /// Destroy this cryptology configuration.
    ~CryptologyConfiguration();

    // defaults/deletions
    CryptologyConfiguration(const CryptologyConfiguration &) = delete;
    CryptologyConfiguration(CryptologyConfiguration &&) = delete;
    auto operator=(const CryptologyConfiguration &) -> CryptologyConfiguration & = delete;
    auto operator=(CryptologyConfiguration &&) -> CryptologyConfiguration & = delete;

public: // accessors
    /// Test whether architecture-specific cryptographic backends may be selected.
    [[nodiscard]] auto hardwareAccelerationEnabled() const -> bool;
    /// Permit or prohibit architecture-specific cryptographic backends for subsequently created workers.
    void setHardwareAccelerationEnabled(bool enabled);
    /// Get the administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm to inspect. Must be a valid hash algorithm.
    /// @return The configured limit, or no value if no limit is configured.
    [[nodiscard]] auto maximumStatus(HashAlgorithm algorithm) const -> std::optional<CryptographicStatus>;
    /// Set an administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm to limit. Must be a valid hash algorithm.
    /// @param status The highest status permitted by the application policy. Must be a valid cryptographic status.
    void setMaximumStatus(HashAlgorithm algorithm, CryptographicStatus status);
    /// Clear the administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm whose limit is cleared. Must be a valid hash algorithm.
    void clearMaximumStatus(HashAlgorithm algorithm);
    /// Get the administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type to inspect. Must be a valid symmetric encryption type.
    /// @return The configured limit, or no value if no limit is configured.
    [[nodiscard]] auto maximumStatus(SymmetricEncryptionType type) const -> std::optional<CryptographicStatus>;
    /// Set an administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type to limit. Must be a valid symmetric encryption type.
    /// @param status The highest status permitted by the application policy. Must be a valid cryptographic status.
    void setMaximumStatus(SymmetricEncryptionType type, CryptographicStatus status);
    /// Clear the administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type whose limit is cleared. Must be a valid symmetric encryption type.
    void clearMaximumStatus(SymmetricEncryptionType type);
    /// Get the configured protected-data provider selection mode.
    [[nodiscard]] auto protectedDataMode() const -> ProtectedDataMode;
    /// Select protected-data provider initialization behavior.
    /// @param mode The provider selection mode. Must be valid and must not differ from the selected mode after provider
    /// initialization.
    void setProtectedDataMode(ProtectedDataMode mode);
    /// Initialize and self-test protected-data support at an explicit application startup point.
    /// @throws CryptologyError If the selected provider cannot be initialized or fails its self-test.
    void validateProtectedDataSupport();

public: // TLS configurations
    /// Maximum number of exact labels in the application-wide TLS registry.
    static constexpr auto cMaximumTlsConfigurations = std::size_t{256U};
    /// Maximum byte length of a TLS configuration label.
    static constexpr auto cMaximumTlsConfigurationLabelLength = impl::tls_configuration_label::cMaximumLength;
    /// Maximum number of slash-delimited segments in a non-empty TLS configuration label.
    static constexpr auto cMaximumTlsConfigurationLabelSegments = impl::tls_configuration_label::cMaximumSegments;

    /// Replace supplied roles at a named label, preserving an existing other role.
    /// @param label Nonempty validated application label; empty is reserved for built-in defaults.
    /// @param configuration One or both complete role configurations.
    /// @throws CryptologyError If a new label exceeds capacity or a supplied role is invalid.
    void setTlsConfiguration(const text::String &label, TlsConfiguration configuration);
    /// Atomically replace all named profiles and shared settings after validating the complete set.
    /// @param configurations The new owned generation; duplicate roles at one label are invalid.
    /// @throws CryptologyError If any profile is invalid, duplicated, or exceeds capacity.
    void replaceTlsConfigurations(const TlsConfigurationSet &configurations);
    /// Get a copy of the current shared TLS settings.
    [[nodiscard]] auto tlsSettings() const -> TlsSettings;
    /// Remove both roles at a named label.
    /// @param label Nonempty application label.
    void clearTlsConfiguration(const text::String &label);
    /// Remove one role at a named label.
    /// @param label Nonempty application label.
    /// @param role The local role to remove.
    void clearTlsConfiguration(const text::String &label, TlsRole role);
    /// Remove named profiles, retaining shared settings and built-in defaults.
    void clearTlsConfigurations();
    /// Test exact role presence, including the reserved built-in defaults.
    /// @param label Valid application label, or empty for the built-in default.
    /// @param role Local connection role.
    /// @return Whether the exact label contains this role.
    [[nodiscard]] auto hasTlsConfiguration(const text::String &label, TlsRole role) const -> bool;
    /// Resolve one role through its label ancestors and reserved default.
    /// @param label Valid application label, or empty for the built-in default.
    /// @param role Local connection role.
    /// @return The selected immutable role and effective certificate policy.
    /// @throws CryptologyError If the selected profile cannot be used for its role.
    [[nodiscard]] auto resolveTlsConfiguration(const text::String &label, TlsRole role) const
        -> TlsConfigurationResolution;
    /// Resolve multiple labels against one coherent generation of the registry.
    /// @param labels Requested labels in result order.
    /// @param role Local connection role.
    /// @return One immutable resolution for each label.
    /// @throws CryptologyError If any selected profile is unusable.
    [[nodiscard]] auto resolveTlsConfigurations(const text::StringList &labels, TlsRole role) const
        -> std::vector<TlsConfigurationResolution>;

public:
    /// Restore acceleration permission and clear status limits and registered TLS configurations.
    void reset();

private:
    /// Capture one coherent view for a complete selector operation.
    [[nodiscard]] auto snapshot() const -> impl::CryptologyConfigurationSnapshot;
    /// Convert a hash algorithm to its configuration array index.
    [[nodiscard]] static auto hashIndex(HashAlgorithm algorithm) -> std::size_t;
    /// Convert a symmetric encryption type to its configuration array index.
    [[nodiscard]] static auto symmetricIndex(SymmetricEncryptionType type) -> std::size_t;
    /// Validate a configured cryptographic status.
    static void validateStatus(CryptographicStatus status);
    /// Initialize the selected protected-data provider. The caller must hold `_mutex`.
    void ensureProtectedDataProvider();
    /// Exercise a provider with one authenticated round trip.
    static void selfTestProtectedDataProvider(impl::ProtectedDataProvider &provider);
    /// Encrypt application-lifetime protected data.
    [[nodiscard]] auto protectData(mem::ConstByteSpan plaintext, unit::ByteCount plaintextLength) -> mem::ByteBlock;
    /// Decrypt application-lifetime protected data.
    [[nodiscard]] auto unprotectData(mem::ConstByteSpan envelope, unit::ByteCount plaintextLength) -> mem::ByteBlock;
    /// Validate a protected-data selection mode.
    static void validateProtectedDataMode(ProtectedDataMode mode);
    /// Validate a named label, rejecting the reserved default.
    static void validateTlsLabel(const text::String &label);
    /// Validate one role and resolve its policy without performing native operations.
    [[nodiscard]] static auto resolveTlsPolicy(
        const TlsConfiguration &configuration, TlsRole role, const TlsSettings &settings)
        -> std::optional<X509TlsCertificatePolicy>;

private:
    mutable std::recursive_mutex _mutex; ///< Protects coherent snapshots, configuration, and provider creation.
    impl::CryptologyConfigurationSnapshot _values;                      ///< Current configuration values.
    ProtectedDataMode _protectedDataMode{ProtectedDataMode::Automatic}; ///< Provider selection before initialization.
    bool _protectedDataModeLocked{false};                  ///< Whether provider selection can no longer change.
    impl::ProtectedDataProviderPtr _protectedDataProvider; ///< Application-lifetime protected-data provider.
    TlsSettings _tlsSettings;                              ///< Shared TLS settings for the current registry generation.
    text::StringHashMap<TlsConfigurationConstPtr> _tlsConfigurations; ///< Immutable labeled TLS configurations.
};

}
