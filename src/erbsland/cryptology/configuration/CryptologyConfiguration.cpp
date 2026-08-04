// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptologyConfiguration.hpp"

#include "../CryptologyError.hpp"
#include "../impl/protected_data/ProtectedDataProvider.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

#include <exception>

namespace erbsland::cryptology {

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
        throw err::LogicError{"Protected-data mode cannot change after provider initialization."_el};
    }
    _protectedDataMode = mode;
}

void CryptologyConfiguration::validateProtectedDataSupport() {
    const auto lock = std::scoped_lock{_mutex};
    _protectedDataModeLocked = true;
    ensureProtectedDataProvider();
}

void CryptologyConfiguration::setTlsConfiguration(const text::String &label, TlsConfiguration configuration) {
    validateTlsConfigurationLabel(label);
    const auto entry = std::make_shared<const TlsConfiguration>(std::move(configuration));
    const auto lock = std::scoped_lock{_mutex};
    if (!_tlsConfigurations.contains(label) && _tlsConfigurations.count().toSizeT() >= cMaximumTlsConfigurations) {
        throw err::RuntimeError{"The application TLS configuration registry is full."_el};
    }
    _tlsConfigurations.set(label, entry);
}

void CryptologyConfiguration::clearTlsConfiguration(const text::String &label) {
    validateTlsConfigurationLabel(label);
    const auto lock = std::scoped_lock{_mutex};
    _tlsConfigurations.remove(label);
}

void CryptologyConfiguration::clearTlsConfigurations() {
    const auto lock = std::scoped_lock{_mutex};
    _tlsConfigurations.clear();
}

auto CryptologyConfiguration::hasTlsConfiguration(const text::String &label) const -> bool {
    validateTlsConfigurationLabel(label);
    const auto lock = std::scoped_lock{_mutex};
    return _tlsConfigurations.contains(label);
}

auto CryptologyConfiguration::resolveTlsConfiguration(const text::String &label) const -> TlsConfigurationResolution {
    validateTlsConfigurationLabel(label);
    const auto lock = std::scoped_lock{_mutex};
    auto candidate = label;
    while (true) {
        if (const auto configuration = _tlsConfigurations.get(candidate); configuration.has_value()) {
            return TlsConfigurationResolution{label, candidate, configuration.value()};
        }
        if (candidate.isEmpty()) {
            break;
        }
        const auto separator = candidate.findLastOf(text::CharSet{"/"_el});
        candidate = separator.isNoIndex() ? text::String{} : candidate.slice(text::StringSide::Front, separator);
    }
    throw err::RuntimeError{tlsConfigurationResolutionError(label)};
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
}

auto CryptologyConfiguration::hashIndex(const HashAlgorithm algorithm) -> std::size_t {
    const auto index = static_cast<std::size_t>(algorithm.toRawValue());
    if (index >= impl::CryptologyConfigurationSnapshot::cHashAlgorithmCount) {
        throw err::ParameterError{"A valid hash algorithm is required."_el, "algorithm"_el};
    }
    return index;
}

auto CryptologyConfiguration::symmetricIndex(const SymmetricEncryptionType type) -> std::size_t {
    const auto index = static_cast<std::size_t>(type.toRawValue());
    if (index == 0U || index >= impl::CryptologyConfigurationSnapshot::cSymmetricTypeStorageCount) {
        throw err::ParameterError{"A valid symmetric encryption type is required."_el, "type"_el};
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
    throw err::ParameterError{"A valid cryptographic status is required."_el, "status"_el};
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
        } catch (...) {
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
    const auto expected = mem::ByteBlock{
        mem::Byte{0x45U},
        mem::Byte{0x72U},
        mem::Byte{0x62U},
        mem::Byte{0x73U},
        mem::Byte{0x6cU},
        mem::Byte{0x61U},
        mem::Byte{0x6eU},
        mem::Byte{0x64U}};
    const auto envelope = provider.protect(expected.span(), expected.length());
    auto actual = provider.unprotect(envelope.span(), expected.length());
    if (!actual.isEqualConstTime(expected)) {
        actual.secureErase();
        throw CryptologyError{"Protected-data provider self-test failed."_el};
    }
    actual.secureErase();
}

auto CryptologyConfiguration::protectData(const mem::ConstByteSpan plaintext, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    const auto lock = std::scoped_lock{_mutex};
    _protectedDataModeLocked = true;
    ensureProtectedDataProvider();
    return _protectedDataProvider->protect(plaintext, plaintextLength);
}

auto CryptologyConfiguration::unprotectData(const mem::ConstByteSpan envelope, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
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
    throw err::ParameterError{"A valid protected-data mode is required."_el, "mode"_el};
}

void CryptologyConfiguration::validateTlsConfigurationLabel(const text::String &label) {
    if (label.length().toSizeT() > cMaximumTlsConfigurationLabelLength) {
        throw err::ParameterError{"A TLS configuration label must not exceed 255 bytes."_el, "label"_el};
    }
    if (label.isEmpty()) {
        return;
    }
    auto segmentCount = std::size_t{1U};
    auto segmentStart = true;
    auto previousWasHyphen = false;
    for (const auto character : label) {
        if (character == U'/') {
            if (segmentStart || previousWasHyphen) {
                throw err::ParameterError{"A TLS configuration label contains an invalid segment."_el, "label"_el};
            }
            ++segmentCount;
            if (segmentCount > cMaximumTlsConfigurationLabelSegments) {
                throw err::ParameterError{"A TLS configuration label must not exceed 16 segments."_el, "label"_el};
            }
            segmentStart = true;
            previousWasHyphen = false;
            continue;
        }
        if (!character.isAsciiLowercaseLetter() && !character.isAsciiDigit() && character != U'-') {
            throw err::ParameterError{"A TLS configuration label contains an invalid character."_el, "label"_el};
        }
        if (segmentStart && character == U'-') {
            throw err::ParameterError{"A TLS configuration label segment cannot begin with a hyphen."_el, "label"_el};
        }
        segmentStart = false;
        previousWasHyphen = character == U'-';
    }
    if (segmentStart || previousWasHyphen) {
        throw err::ParameterError{"A TLS configuration label contains an invalid segment."_el, "label"_el};
    }
}

auto CryptologyConfiguration::tlsConfigurationResolutionError(const text::String &label) -> text::String {
    auto result = text::StringEditor{"No TLS configuration matched the fallback chain: "_el};
    auto candidate = label;
    auto first = true;
    while (true) {
        if (!first) {
            result.append(" -> "_el);
        }
        result.append(candidate.isEmpty() ? "\"\""_el : candidate);
        if (candidate.isEmpty()) {
            break;
        }
        const auto separator = candidate.findLastOf(text::CharSet{"/"_el});
        candidate = separator.isNoIndex() ? text::String{} : candidate.slice(text::StringSide::Front, separator);
        first = false;
    }
    return result;
}

}
