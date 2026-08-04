// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricCipher.hpp"

#include "../CryptographicSecurity.hpp"
#include "../CryptographicStatus.hpp"

#include <optional>

namespace erbsland::cryptology {

/// Requirements for selecting a symmetric encryption type.
/// The defaults request an accepted authenticated type with at least standard security.
/// @seedoc{/reference/cryptology/symmetric_encryption}
/// @tested{SymmetricEncryptionTypeTest}
struct SymmetricEncryptionRequirements final {
    CryptographicStatus requiredStatus{CryptographicStatus::Acceptable};    ///< The exact required status.
    CryptographicSecurity minimumSecurity{CryptographicSecurity::Standard}; ///< The minimum security level.
    bool requireAead{true};                                                 ///< Require authenticated encryption.
    std::optional<SymmetricCipher> requiredCipher;                          ///< The required cipher family, if any.
};

}
