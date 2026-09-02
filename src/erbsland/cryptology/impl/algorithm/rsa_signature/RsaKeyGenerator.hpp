// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteBlock_fwd.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl::rsa_key_generator {

/// Generate and validate one two-prime RSA private key using fixed-width secret storage.
///
/// Generation follows FIPS 186-5 sections 5.1 and A.1 and uses the probabilistic primality test from section B.3.1
/// with 25 independently random bases. The output is a PKCS#1 `RSAPrivateKey` as specified by RFC 8017 appendix A.1.2.
/// @tested{X509CertificateBuilderTest}
[[nodiscard]] auto generate(std::size_t modulusBits) -> mem::ByteBlock;

}
