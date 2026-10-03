// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptologyConfiguration.hpp"

#include "../CryptologyError.hpp"
#include "../impl/protected_data/ProtectedDataProvider.hpp"
#include "../impl/TlsConfigurationLabel.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringList.hpp"
#include "../../text/StringSide.hpp"

#include <exception>

namespace erbsland::cryptology {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;

CryptologyConfiguration::CryptologyConfiguration() {
    reset();
}

CryptologyConfiguration::~CryptologyConfiguration() = default;

auto CryptologyConfiguration::hardwareAccelerationEnabled() const -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _values.hardwareAccelerationEnabled();
}

void CryptologyConfiguration::setHardwareAccelerationEnabled(const bool enabled) {
    const auto lock = std::scoped_lock{_mutex};
    _values._hardwareAccelerationEnabled = enabled;
}

auto CryptologyConfiguration::maximumStatus(const HashAlgorithm algorithm) const -> std::optional<CryptographicStatus> {
    const auto index = hashIndex(algorithm);
    const auto lock = std::scoped_lock{_mutex};
    const auto value = _values._hashMaximumStatus[index];
    return value == impl::CryptologyConfigurationSnapshot::cNoStatusLimit
        ? std::nullopt
        : std::optional{static_cast<CryptographicStatus>(value)};
}

void CryptologyConfiguration::setMaximumStatus(const HashAlgorithm algorithm, const CryptographicStatus status) {
    const auto index = hashIndex(algorithm);
    validateStatus(status);
    const auto lock = std::scoped_lock{_mutex};
    _values._hashMaximumStatus[index] = static_cast<uint8_t>(status);
}

void CryptologyConfiguration::clearMaximumStatus(const HashAlgorithm algorithm) {
    const auto index = hashIndex(algorithm);
    const auto lock = std::scoped_lock{_mutex};
    _values._hashMaximumStatus[index] = impl::CryptologyConfigurationSnapshot::cNoStatusLimit;
}

auto CryptologyConfiguration::maximumStatus(const SymmetricEncryptionType type) const
    -> std::optional<CryptographicStatus> {
    const auto index = symmetricIndex(type);
    const auto lock = std::scoped_lock{_mutex};
    const auto value = _values._symmetricMaximumStatus[index];
    return value == impl::CryptologyConfigurationSnapshot::cNoStatusLimit
        ? std::nullopt
        : std::optional{static_cast<CryptographicStatus>(value)};
}

void CryptologyConfiguration::setMaximumStatus(const SymmetricEncryptionType type, const CryptographicStatus status) {
    const auto index = symmetricIndex(type);
    validateStatus(status);
    const auto lock = std::scoped_lock{_mutex};
    _values._symmetricMaximumStatus[index] = static_cast<uint8_t>(status);
}

void CryptologyConfiguration::clearMaximumStatus(const SymmetricEncryptionType type) {
    const auto index = symmetricIndex(type);
    const auto lock = std::scoped_lock{_mutex};
    _values._symmetricMaximumStatus[index] = impl::CryptologyConfigurationSnapshot::cNoStatusLimit;
}

auto CryptologyConfiguration::protectedDataMode() const -> ProtectedDataMode {
    const auto lock = std::scoped_lock{_mutex};
    return _protectedDataMode;
}

void CryptologyConfiguration::setProtectedDataMode(const ProtectedDataMode mode) {
    validateProtectedDataMode(mode);
    const auto lock = std::scoped_lock{_mutex};
    if (_protectedDataModeLocked && mode != _protectedDataMode) {
        throw LogicFailure{"Protected-data mode cannot change after provider initialization."_el};
    }
    _protectedDataMode = mode;
}

void CryptologyConfiguration::validateProtectedDataSupport() {
    const auto lock = std::scoped_lock{_mutex};
    _protectedDataModeLocked = true;
    ensureProtectedDataProvider();
}

void CryptologyConfiguration::validateTlsLabel(const String &label) {
    impl::tls_configuration_label::validate(label);
    if (label.isEmpty()) {
        throw ParameterFailure{"The empty TLS label is reserved for built-in defaults."_el, "label"_el};
    }
}

auto CryptologyConfiguration::resolveTlsPolicy(
    const TlsConfiguration &configuration, const TlsRole role, const TlsSettings &settings)
    -> std::optional<X509TlsCertificatePolicy> {
    if (role == TlsRole::Client) {
        return configuration.client()->trust().resolve(settings, role);
    }
    const auto &server = *configuration.server();
    if (server.identity() == nullptr) {
        throw CryptologyError{"A TLS server profile requires a server certificate identity."_el};
    }
    const auto authentication = server.clientAuthentication();
    if (authentication != TlsClientAuthentication::Disabled && authentication != TlsClientAuthentication::Optional &&
        authentication != TlsClientAuthentication::Required) {
        throw ParameterFailure{"A valid TLS client authentication mode is required."_el, "clientAuthentication"_el};
    }
    return server.trust().resolve(settings, role, authentication != TlsClientAuthentication::Disabled);
}

void CryptologyConfiguration::setTlsConfiguration(const String &label, TlsConfiguration configuration) {
    validateTlsLabel(label);
    const auto lock = std::scoped_lock{_mutex};
    if (!configuration.client() && !configuration.server()) {
        throw ParameterFailure{"A TLS profile must contain at least one role."_el, "configuration"_el};
    }
    for (const auto role : {TlsRole::Client, TlsRole::Server}) {
        if (configuration.hasRole(role)) {
            [[maybe_unused]] const auto policy = resolveTlsPolicy(configuration, role, _tlsSettings);
        }
    }
    if (const auto existing = _tlsConfigurations.get(label); existing) {
        auto combined = **existing;
        combined.merge(configuration);
        configuration = std::move(combined);
    } else if (_tlsConfigurations.count().toSizeT() >= cMaximumTlsConfigurations) {
        throw CryptologyError{"The application TLS configuration registry is full."_el};
    }
    _tlsConfigurations.set(label, std::make_shared<const TlsConfiguration>(std::move(configuration)));
}

void CryptologyConfiguration::replaceTlsConfigurations(const TlsConfigurationSet &configurations) {
    auto replacement = StringHashMap<TlsConfigurationConstPtr>{};
    for (const auto &entry : configurations.entries()) {
        validateTlsLabel(entry.label());
        const auto &supplied = entry.configuration();
        if (!supplied.client() && !supplied.server()) {
            throw CryptologyError{"A named TLS profile must contain at least one role."_el};
        }
        auto combined = TlsConfiguration{};
        if (const auto existing = replacement.get(entry.label()); existing) {
            combined = **existing;
        }
        for (const auto role : {TlsRole::Client, TlsRole::Server}) {
            if (!supplied.hasRole(role)) {
                continue;
            }
            if (combined.hasRole(role)) {
                throw CryptologyError{"Duplicate TLS profile role at one label."_el};
            }
            [[maybe_unused]] const auto policy = resolveTlsPolicy(supplied, role, configurations.settings());
        }
        combined.merge(supplied);
        replacement.set(entry.label(), std::make_shared<const TlsConfiguration>(std::move(combined)));
        if (replacement.count().toSizeT() > cMaximumTlsConfigurations) {
            throw CryptologyError{"The application TLS configuration registry is full."_el};
        }
    }
    auto settings = configurations.settings();
    const auto lock = std::scoped_lock{_mutex};
    _tlsSettings = std::move(settings);
    _tlsConfigurations = std::move(replacement);
}

auto CryptologyConfiguration::tlsSettings() const -> TlsSettings {
    const auto lock = std::scoped_lock{_mutex};
    return _tlsSettings;
}

void CryptologyConfiguration::clearTlsConfiguration(const String &label) {
    validateTlsLabel(label);
    const auto lock = std::scoped_lock{_mutex};
    _tlsConfigurations.remove(label);
}

void CryptologyConfiguration::clearTlsConfiguration(const String &label, const TlsRole role) {
    validateTlsLabel(label);
    TlsConfiguration::validateRole(role);
    const auto lock = std::scoped_lock{_mutex};
    if (const auto existing = _tlsConfigurations.get(label); existing) {
        auto configuration = **existing;
        configuration.clearRole(role);
        if (!configuration.client() && !configuration.server()) {
            _tlsConfigurations.remove(label);
        } else {
            _tlsConfigurations.set(label, std::make_shared<const TlsConfiguration>(std::move(configuration)));
        }
    }
}

void CryptologyConfiguration::clearTlsConfigurations() {
    const auto lock = std::scoped_lock{_mutex};
    _tlsConfigurations.clear();
}

auto CryptologyConfiguration::hasTlsConfiguration(const String &label, const TlsRole role) const -> bool {
    impl::tls_configuration_label::validate(label);
    TlsConfiguration::validateRole(role);
    const auto lock = std::scoped_lock{_mutex};
    if (label.isEmpty()) {
        return true;
    }
    const auto entry = _tlsConfigurations.get(label);
    return entry && (*entry)->hasRole(role);
}

auto CryptologyConfiguration::resolveTlsConfiguration(const String &label, const TlsRole role) const
    -> TlsConfigurationResolution {
    impl::tls_configuration_label::validate(label);
    TlsConfiguration::validateRole(role);
    const auto lock = std::scoped_lock{_mutex};
    auto candidate = label;
    auto selected = TlsConfigurationConstPtr{};
    while (!candidate.isEmpty()) {
        if (const auto entry = _tlsConfigurations.get(candidate); entry && (*entry)->hasRole(role)) {
            selected = *entry;
            break;
        }
        if (const auto parts = candidate.splitAtLast(CharSet{U'/'})) {
            candidate = parts->first;
        } else {
            candidate = {};
        }
    }
    if (selected == nullptr) {
        static const auto cClientDefault = std::make_shared<const TlsConfiguration>(TlsClientConfiguration{});
        static const auto cServerDefault = std::make_shared<const TlsConfiguration>(TlsServerConfiguration{});
        if (role == TlsRole::Client) {
            selected = cClientDefault;
        } else {
            selected = cServerDefault;
        }
    }
    auto policy = resolveTlsPolicy(*selected, role, _tlsSettings);
    return TlsConfigurationResolution{label, candidate, std::move(selected), role, std::move(policy)};
}

auto CryptologyConfiguration::resolveTlsConfigurations(const StringList &labels, const TlsRole role) const
    -> std::vector<TlsConfigurationResolution> {
    const auto lock = std::scoped_lock{_mutex};
    auto result = std::vector<TlsConfigurationResolution>{};
    result.reserve(labels.count().toSizeT());
    for (const auto &label : labels) {
        result.push_back(resolveTlsConfiguration(label, role));
    }
    return result;
}

auto CryptologyConfiguration::snapshot() const -> impl::CryptologyConfigurationSnapshot {
    const auto lock = std::scoped_lock{_mutex};
    return _values;
}

void CryptologyConfiguration::reset() {
    const auto lock = std::scoped_lock{_mutex};
    _values._hardwareAccelerationEnabled = true;
    _values._hashMaximumStatus.fill(impl::CryptologyConfigurationSnapshot::cNoStatusLimit);
    _values._symmetricMaximumStatus.fill(impl::CryptologyConfigurationSnapshot::cNoStatusLimit);
    if (!_protectedDataModeLocked) {
        _protectedDataMode = ProtectedDataMode::Automatic;
    }
    _tlsConfigurations.clear();
    _tlsSettings = TlsSettings{};
}

auto CryptologyConfiguration::hashIndex(const HashAlgorithm algorithm) -> std::size_t {
    const auto index = static_cast<std::size_t>(algorithm.toRawValue());
    if (index >= impl::CryptologyConfigurationSnapshot::cHashAlgorithmCount) {
        throw ParameterFailure{"A valid hash algorithm is required."_el, "algorithm"_el};
    }
    return index;
}

auto CryptologyConfiguration::symmetricIndex(const SymmetricEncryptionType type) -> std::size_t {
    const auto index = static_cast<std::size_t>(type.toRawValue());
    if (index == 0U || index >= impl::CryptologyConfigurationSnapshot::cSymmetricTypeStorageCount) {
        throw ParameterFailure{"A valid symmetric encryption type is required."_el, "type"_el};
    }
    return index;
}

void CryptologyConfiguration::validateStatus(const CryptographicStatus status) {
    switch (status) {
    case CryptographicStatus::Disallowed:
    case CryptographicStatus::Legacy:
    case CryptographicStatus::Acceptable:
        return;
    }
    throw ParameterFailure{"A valid cryptographic status is required."_el, "status"_el};
}

void CryptologyConfiguration::ensureProtectedDataProvider() {
    if (_protectedDataProvider != nullptr) {
        return;
    }
    if (_protectedDataMode != ProtectedDataMode::InternalOnly) {
        try {
            auto platformProvider = impl::ProtectedDataProvider::createPlatform();
            if (platformProvider != nullptr) {
                selfTestProtectedDataProvider(*platformProvider);
                _protectedDataProvider = std::move(platformProvider);
                return;
            }
        } catch (const CryptologyError &) {
            if (_protectedDataMode == ProtectedDataMode::PlatformOnly) {
                throw;
            }
        } catch (const RuntimeError &) {
            if (_protectedDataMode == ProtectedDataMode::PlatformOnly) {
                throw CryptologyError{
                    "Native protected-data provider initialization failed."_el, std::current_exception()};
            }
        }
        if (_protectedDataMode == ProtectedDataMode::PlatformOnly) {
            throw CryptologyError{"This platform has no native protected-data provider."_el};
        }
    }
    auto internalProvider = impl::ProtectedDataProvider::createInternal();
    selfTestProtectedDataProvider(*internalProvider);
    _protectedDataProvider = std::move(internalProvider);
}

void CryptologyConfiguration::selfTestProtectedDataProvider(impl::ProtectedDataProvider &provider) {
    const auto expected = ByteBlock{
        Byte{0x45U}, Byte{0x72U}, Byte{0x62U}, Byte{0x73U}, Byte{0x6cU}, Byte{0x61U}, Byte{0x6eU}, Byte{0x64U}};
    const auto envelope = provider.protect(expected.span(), expected.length());
    auto actual = provider.unprotect(envelope.span(), expected.length());
    if (!actual.isEqualConstTime(expected)) {
        actual.secureErase();
        throw CryptologyError{"Protected-data provider self-test failed."_el};
    }
    actual.secureErase();
}

auto CryptologyConfiguration::protectData(const ConstByteSpan plaintext, const unit::ByteCount plaintextLength)
    -> ByteBlock {
    const auto lock = std::scoped_lock{_mutex};
    _protectedDataModeLocked = true;
    ensureProtectedDataProvider();
    return _protectedDataProvider->protect(plaintext, plaintextLength);
}

auto CryptologyConfiguration::unprotectData(const ConstByteSpan envelope, const unit::ByteCount plaintextLength)
    -> ByteBlock {
    const auto lock = std::scoped_lock{_mutex};
    _protectedDataModeLocked = true;
    ensureProtectedDataProvider();
    return _protectedDataProvider->unprotect(envelope, plaintextLength);
}

void CryptologyConfiguration::validateProtectedDataMode(const ProtectedDataMode mode) {
    switch (mode) {
    case ProtectedDataMode::Automatic:
    case ProtectedDataMode::PlatformOnly:
    case ProtectedDataMode::InternalOnly:
        return;
    }
    throw ParameterFailure{"A valid protected-data mode is required."_el, "mode"_el};
}

}
