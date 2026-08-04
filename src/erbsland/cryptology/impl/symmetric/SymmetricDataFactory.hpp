// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricDecryptorData_fwd.hpp"
#include "SymmetricEncryptorData_fwd.hpp"

#include "../../symmetric/SymmetricEncryptionType.hpp"
#include "../../symmetric/SymmetricIv.hpp"
#include "../../symmetric/SymmetricKey.hpp"
#include "../../symmetric/SymmetricNonce.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Create an AEAD encryption worker.
/// @param type The authenticated encryption construction.
/// @param key The validated secret key.
/// @param nonce The validated nonce.
/// @return A new worker in its initial state.
/// @throws CryptologyError If the requested construction is unavailable.
/// @tested{AesGcmTest SymmetricEncryptionFrontendTest}
auto createSymmetricEncryptorData(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce)
    -> std::unique_ptr<SymmetricEncryptorData>;

/// Create an IV-based encryption worker.
/// @param type The unauthenticated encryption construction.
/// @param key The validated secret key.
/// @param iv The validated initialization vector.
/// @return A new worker in its initial state.
/// @throws CryptologyError If the requested construction is unavailable.
/// @tested{AesCbcTest SymmetricEncryptionFrontendTest}
auto createSymmetricEncryptorData(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv)
    -> std::unique_ptr<SymmetricEncryptorData>;

/// Create an AEAD decryption worker.
/// @param type The authenticated encryption construction.
/// @param key The validated secret key.
/// @param nonce The validated nonce.
/// @return A new worker in its initial state.
/// @throws CryptologyError If the requested construction is unavailable.
/// @tested{AesGcmTest SymmetricEncryptionFrontendTest}
auto createSymmetricDecryptorData(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce)
    -> std::unique_ptr<SymmetricDecryptorData>;

/// Create an IV-based decryption worker.
/// @param type The unauthenticated encryption construction.
/// @param key The validated secret key.
/// @param iv The validated initialization vector.
/// @return A new worker in its initial state.
/// @throws CryptologyError If the requested construction is unavailable.
/// @tested{AesCbcTest SymmetricEncryptionFrontendTest}
auto createSymmetricDecryptorData(SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv)
    -> std::unique_ptr<SymmetricDecryptorData>;

}
