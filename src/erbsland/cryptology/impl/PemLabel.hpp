// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology::impl {

/// The supported RFC 7468 textual labels.
enum class PemLabel : uint8_t {
    Certificate,         ///< X.509 certificate.
    PrivateKey,          ///< Unencrypted PKCS#8 private key.
    EncryptedPrivateKey, ///< Encrypted PKCS#8 private key.
    PublicKey,           ///< SubjectPublicKeyInfo public key.
    CertificateRequest,  ///< PKCS#10 certificate signing request.
};

}
