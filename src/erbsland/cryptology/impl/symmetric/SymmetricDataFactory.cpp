// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SymmetricDataFactory.hpp"

#include "AesCbcDecryptorData.hpp"
#include "AesCbcEncryptorData.hpp"
#include "AesGcmDecryptorData.hpp"
#include "AesGcmEncryptorData.hpp"
#include "ChaCha20Poly1305DecryptorData.hpp"
#include "ChaCha20Poly1305EncryptorData.hpp"
#include "UnsafeSymmetricKeyAccess.hpp"

#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto createSymmetricEncryptorData(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce)
    -> std::unique_ptr<SymmetricEncryptorData> {
    switch (type.toRawValue()) {
    case SymmetricEncryptionType::Aes128Gcm:
    case SymmetricEncryptionType::Aes256Gcm:
        return std::make_unique<AesGcmEncryptorData>(type, UnsafeSymmetricKeyAccess{key}.span(), nonce.span());
    case SymmetricEncryptionType::ChaCha20Poly1305:
        return std::make_unique<ChaCha20Poly1305EncryptorData>(UnsafeSymmetricKeyAccess{key}.span(), nonce.span());
    default:
        throw CryptologyError{"The selected symmetric encryption type does not use a nonce."_el};
    }
}

auto createSymmetricEncryptorData(const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv)
    -> std::unique_ptr<SymmetricEncryptorData> {
    switch (type.toRawValue()) {
    case SymmetricEncryptionType::Aes256CbcRandomFill:
    case SymmetricEncryptionType::Aes256CbcIso9797Method2:
        return std::make_unique<AesCbcEncryptorData>(type, UnsafeSymmetricKeyAccess{key}.span(), iv.span());
    default:
        throw CryptologyError{"The selected symmetric encryption type does not use an initialization vector."_el};
    }
}

auto createSymmetricDecryptorData(
    const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricNonce &nonce)
    -> std::unique_ptr<SymmetricDecryptorData> {
    switch (type.toRawValue()) {
    case SymmetricEncryptionType::Aes128Gcm:
    case SymmetricEncryptionType::Aes256Gcm:
        return std::make_unique<AesGcmDecryptorData>(type, UnsafeSymmetricKeyAccess{key}.span(), nonce.span());
    case SymmetricEncryptionType::ChaCha20Poly1305:
        return std::make_unique<ChaCha20Poly1305DecryptorData>(UnsafeSymmetricKeyAccess{key}.span(), nonce.span());
    default:
        throw CryptologyError{"The selected symmetric encryption type does not use a nonce."_el};
    }
}

auto createSymmetricDecryptorData(const SymmetricEncryptionType type, const SymmetricKey &key, const SymmetricIv &iv)
    -> std::unique_ptr<SymmetricDecryptorData> {
    switch (type.toRawValue()) {
    case SymmetricEncryptionType::Aes256CbcRandomFill:
    case SymmetricEncryptionType::Aes256CbcIso9797Method2:
        return std::make_unique<AesCbcDecryptorData>(type, UnsafeSymmetricKeyAccess{key}.span(), iv.span());
    default:
        throw CryptologyError{"The selected symmetric encryption type does not use an initialization vector."_el};
    }
}

}
