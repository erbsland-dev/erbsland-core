// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSignature.hpp"

namespace erbsland::cryptology::impl::rsa_signature {

auto rsaVerificationPrimitive(const PublicKeyData &key, const mem::ConstByteSpan signature)
    -> std::optional<mem::ByteBlock> {
    // RFC 8017 section 8.1.2 step 1 and section 8.2.2 step 1 require one signature of exactly k octets.
    if (signature.size() != key.encodedLength) {
        return std::nullopt;
    }

    // RFC 8017 sections 4.2 and 5.2.2 step 1: OS2IP converts S to `s`, which must satisfy 0 <= s < n.
    const auto signatureRepresentative = Number::fromBigEndian(signature, key.modulus.wordCount());
    if (signatureRepresentative.compare(key.modulus) >= 0) {
        return std::nullopt;
    }

    // RFC 8017 section 5.2.2 step 2 applies RSAVP1: `m = s^e mod n`.
    // Square-and-multiply branches only on the public exponent and operates solely on public values.
    auto messageRepresentative = Number::fromValue(1U, key.modulus.wordCount());
    for (const auto exponentByte : key.exponent.span()) {
        for (auto mask = uint8_t{0x80U}; mask != 0U; mask >>= 1U) {
            messageRepresentative = messageRepresentative.multipliedModulo(messageRepresentative, key.modulus);
            if ((exponentByte.toUInt8() & mask) != 0U) {
                messageRepresentative = messageRepresentative.multipliedModulo(signatureRepresentative, key.modulus);
            }
        }
    }

    // RFC 8017 sections 4.1 and 5.2.2 step 3: I2OSP converts `m` to the fixed k-octet encoded message.
    return messageRepresentative.toBigEndian(key.encodedLength);
}

}
