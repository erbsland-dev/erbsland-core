// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CryptologyConfigurationSnapshot.hpp"

namespace erbsland::cryptology::impl {

auto CryptologyConfigurationSnapshot::maximumStatus(const HashAlgorithm algorithm) const noexcept
    -> std::optional<CryptographicStatus> {
    const auto value = static_cast<std::size_t>(algorithm.toRawValue());
    if (value >= _hashMaximumStatus.size() || _hashMaximumStatus[value] == cNoStatusLimit) {
        return std::nullopt;
    }
    return static_cast<CryptographicStatus>(_hashMaximumStatus[value]);
}

auto CryptologyConfigurationSnapshot::maximumStatus(const SymmetricEncryptionType type) const noexcept
    -> std::optional<CryptographicStatus> {
    const auto value = static_cast<std::size_t>(type.toRawValue());
    if (value == 0U || value >= _symmetricMaximumStatus.size() || _symmetricMaximumStatus[value] == cNoStatusLimit) {
        return std::nullopt;
    }
    return static_cast<CryptographicStatus>(_symmetricMaximumStatus[value]);
}

}
