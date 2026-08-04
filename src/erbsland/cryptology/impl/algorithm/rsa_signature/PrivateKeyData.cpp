// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSigner.hpp"

namespace erbsland::cryptology::impl::rsa_signer {

void PrivateKeyData::secureErase() noexcept {
    modulus.secureErase();
    publicExponent.secureErase();
    privateExponent.secureErase();
    prime1.secureErase();
    prime2.secureErase();
    exponent1.secureErase();
    exponent2.secureErase();
    coefficient.secureErase();
    modulusBits = 0U;
    encodedLength = 0U;
}

}
