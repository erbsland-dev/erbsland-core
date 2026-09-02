// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CryptographicStatus.hpp"
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

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/StringHashMap.hpp"
#include "../../unit/ByteLength.hpp"

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
    /// @param algorithm The algorithm to inspect.
    /// @return The configured limit, or no value if no limit is configured.
    /// @throws err::ParameterError If `algorithm` is invalid.
    [[nodiscard]] auto maximumStatus(HashAlgorithm algorithm) const -> std::optional<CryptographicStatus>;
    /// Set an administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm to limit.
    /// @param status The highest status permitted by the application policy.
    /// @throws err::ParameterError If either value is invalid.
    void setMaximumStatus(HashAlgorithm algorithm, CryptographicStatus status);
    /// Clear the administrative maximum status for a hash algorithm.
    /// @param algorithm The algorithm whose limit is cleared.
    /// @throws err::ParameterError If `algorithm` is invalid.
    void clearMaximumStatus(HashAlgorithm algorithm);
    /// Get the administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type to inspect.
    /// @return The configured limit, or no value if no limit is configured.
    /// @throws err::ParameterError If `type` is invalid.
    [[nodiscard]] auto maximumStatus(SymmetricEncryptionType type) const -> std::optional<CryptographicStatus>;
    /// Set an administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type to limit.
    /// @param status The highest status permitted by the application policy.
    /// @throws err::ParameterError If either value is invalid.
    void setMaximumStatus(SymmetricEncryptionType type, CryptographicStatus status);
    /// Clear the administrative maximum status for a symmetric encryption type.
    /// @param type The encryption type whose limit is cleared.
    /// @throws err::ParameterError If `type` is invalid.
    void clearMaximumStatus(SymmetricEncryptionType type);
    /// Get the configured protected-data provider selection mode.
    [[nodiscard]] auto protectedDataMode() const -> ProtectedDataMode;
    /// Select protected-data provider initialization behavior.
    /// @param mode The provider selection mode.
    /// @throws err::ParameterError If `mode` is invalid.
    /// @throws err::LogicError If a different mode is selected after provider initialization.
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

    /// Atomically register or replace one TLS configuration.
    /// @param label The exact hierarchical label, or an empty label for the global default.
    /// @param configuration The configuration copied into a new immutable registry entry.
    /// @throws err::ParameterError If the label is invalid.
    /// @throws err::RuntimeError If a new label would exceed the registry capacity.
    void setTlsConfiguration(const text::String &label, TlsConfiguration configuration);
    /// Remove one exact TLS configuration label.
    /// @param label The exact hierarchical label, or the empty global-default label.
    /// @throws err::ParameterError If the label is invalid.
    void clearTlsConfiguration(const text::String &label);
    /// Remove every registered TLS configuration.
    void clearTlsConfigurations();
    /// Test whether one exact TLS configuration label is registered.
    /// Parent fallbacks are intentionally not considered.
    /// @param label The exact hierarchical label, or the empty global-default label.
    /// @return `true` if the exact label is registered.
    /// @throws err::ParameterError If the label is invalid.
    [[nodiscard]] auto hasTlsConfiguration(const text::String &label) const -> bool;
    /// Resolve a label to one complete immutable TLS configuration.
    /// Resolution removes trailing slash-delimited segments and finally tries the empty global-default label.
    /// @param label The exact or descendant label to resolve.
    /// @return The requested label, matched label, and immutable selected entry.
    /// @throws err::ParameterError If the label is invalid.
    /// @throws err::RuntimeError If no fallback candidate is registered.
    [[nodiscard]] auto resolveTlsConfiguration(const text::String &label) const -> TlsConfigurationResolution;

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
    [[nodiscard]] auto protectData(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength) -> mem::ByteBlock;
    /// Decrypt application-lifetime protected data.
    [[nodiscard]] auto unprotectData(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength) -> mem::ByteBlock;
    /// Validate a protected-data selection mode.
    static void validateProtectedDataMode(ProtectedDataMode mode);
    /// Build a diagnostic containing every fallback candidate for a label.
    [[nodiscard]] static auto tlsConfigurationResolutionError(const text::String &label) -> text::String;

private:
    mutable std::recursive_mutex _mutex; ///< Protects coherent snapshots, configuration, and provider creation.
    impl::CryptologyConfigurationSnapshot _values;                      ///< Current configuration values.
    ProtectedDataMode _protectedDataMode{ProtectedDataMode::Automatic}; ///< Provider selection before initialization.
    bool _protectedDataModeLocked{false};                  ///< Whether provider selection can no longer change.
    impl::ProtectedDataProviderPtr _protectedDataProvider; ///< Application-lifetime protected-data provider.
    text::StringHashMap<TlsConfigurationConstPtr> _tlsConfigurations; ///< Immutable labeled TLS configurations.
};

}
