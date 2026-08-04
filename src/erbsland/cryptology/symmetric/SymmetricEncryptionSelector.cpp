// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SymmetricEncryptionSelector.hpp"

#include "../../core/Application.hpp"
#include "../../util/List.hpp"

#include <algorithm>

namespace erbsland::cryptology {

auto SymmetricEncryptionSelector::matches(const SymmetricEncryptionType type) const -> bool {
    return matches(type, core::application().cryptologyConfiguration().snapshot());
}

auto SymmetricEncryptionSelector::status(const SymmetricEncryptionType type) const -> CryptographicStatus {
    return effectiveStatus(type, core::application().cryptologyConfiguration().snapshot());
}

auto SymmetricEncryptionSelector::allAccepted() const -> util::List<SymmetricEncryptionType> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    auto result = util::List<SymmetricEncryptionType>{};
    for (const auto type : SymmetricEncryptionType::all()) {
        if (effectiveStatus(type, configuration) == CryptographicStatus::Acceptable) {
            result.append(type);
        }
    }
    return result;
}

auto SymmetricEncryptionSelector::matching() const -> util::List<SymmetricEncryptionType> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    auto result = util::List<SymmetricEncryptionType>{};
    for (const auto type : SymmetricEncryptionType::all()) {
        if (matches(type, configuration)) {
            result.append(type);
        }
    }
    return result;
}

auto SymmetricEncryptionSelector::recommended() const -> std::optional<SymmetricEncryptionType> {
    const auto configuration = core::application().cryptologyConfiguration().snapshot();
    for (const auto type : SymmetricEncryptionType::all()) {
        if (matches(type, configuration)) {
            return type;
        }
    }
    return std::nullopt;
}

auto SymmetricEncryptionSelector::libraryStatus(const SymmetricEncryptionType type) noexcept -> CryptographicStatus {
    switch (type.toRawValue()) {
    case SymmetricEncryptionType::Aes256Gcm:
    case SymmetricEncryptionType::ChaCha20Poly1305:
    case SymmetricEncryptionType::Aes128Gcm:
        return CryptographicStatus::Acceptable;
    case SymmetricEncryptionType::Aes256CbcRandomFill:
    case SymmetricEncryptionType::Aes256CbcIso9797Method2:
        return CryptographicStatus::Legacy;
    case SymmetricEncryptionType::None:
        return CryptographicStatus::Disallowed;
    }
    return CryptographicStatus::Disallowed;
}

auto SymmetricEncryptionSelector::effectiveStatus(
    const SymmetricEncryptionType type, const impl::CryptologyConfigurationSnapshot &configuration) noexcept
    -> CryptographicStatus {
    const auto baseStatus = libraryStatus(type);
    const auto maximumStatus = configuration.maximumStatus(type);
    return maximumStatus.has_value() ? std::min(baseStatus, maximumStatus.value()) : baseStatus;
}

auto SymmetricEncryptionSelector::matches(
    const SymmetricEncryptionType type, const impl::CryptologyConfigurationSnapshot &configuration) const noexcept
    -> bool {
    return type.isValid() && effectiveStatus(type, configuration) == _requirements.requiredStatus &&
        type.security() >= _requirements.minimumSecurity && (!_requirements.requireAead || type.isAead()) &&
        (!_requirements.requiredCipher.has_value() || type.cipher() == _requirements.requiredCipher.value());
}

}
