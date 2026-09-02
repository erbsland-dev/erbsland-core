// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../keys/SigningPrivateKey_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::encrypted_private_key_codec {

/// Strict EncryptedPrivateKeyInfo codec for the library PBES2 profile.
///
/// The profile follows RFC 8018 sections 5.2, 6.2, and appendix A.4: PBES2 with PBKDF2-HMAC-SHA256 and AES-256-CBC.
/// PKCS#7 padding follows RFC 5652 section 6.3. Plaintext, password copies, derived keys, and padded buffers remain in
/// sensitive storage and are erased on every exit path.
/// @tested{X509CertificateBuilderTest}
constexpr auto cIterationCount = uint32_t{1'000'000U};
constexpr auto cMaximumIterationCount = uint32_t{10'000'000U};

/// Encrypt canonical PKCS#8 DER with fresh salt and IV.
[[nodiscard]] auto encrypt(mem::ConstByteSpan plaintext, mem::ConstByteSpan password) -> mem::ByteBlock;
/// Decrypt and validate one supported EncryptedPrivateKeyInfo value.
[[nodiscard]] auto decrypt(const mem::ByteBlock &der, mem::ConstByteSpan password) -> mem::ByteBlock;

}
