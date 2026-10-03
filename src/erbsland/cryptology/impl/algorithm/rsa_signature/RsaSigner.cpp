// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSigner.hpp"

#include "RsaSigning.hpp"

#include "../../../../core/impl/application_data/ApplicationRandomData.hpp"
#include "../../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../../random/Random.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../CryptologyError.hpp"
#include "../../../tls/TlsSignatureScheme.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::rsa_signer {

using namespace mem;
using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

auto sign(
    const ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const ConstByteSpan message,
    const PublicKey &publicKeyValue) -> ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};
    const auto hash = signing::hashForScheme(scheme);

    // RFC 8446 section 4.2.3: use a digest-sized random PSS salt and one modulus-sized message-blinding factor.
    const auto randomData = core::impl::applicationData()->random().get();
    auto salt = randomData->secureRandom().buildByteBlock(hash.digestSize());
    salt.markAsSensitive();
    const auto saltEraseGuard = SecureEraseGuard{salt};
    for (auto attempt = 0U; attempt < 64U; ++attempt) {
        auto blindingFactor = randomData->secureRandom().buildByteBlock(ByteCount::fromSizeT(key.encodedLength));
        blindingFactor.markAsSensitive();
        const auto blindingFactorEraseGuard = SecureEraseGuard{blindingFactor};
        auto r = Number::fromBigEndian(blindingFactor.span(), key.modulus.wordCount());
        const auto rEraseGuard = SecureEraseGuard{r};
        if (!r.isZero() && r.compare(key.modulus) < 0) {
            return signing::signDecoded(key, scheme, message, publicKeyValue, salt.span(), blindingFactor.span());
        }
    }
    throw CryptologyError{"Secure randomness did not produce a valid RSA blinding factor within the fixed bound."_el};
}

auto signWithRandom(
    const ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const ConstByteSpan message,
    const PublicKey &publicKeyValue,
    const ConstByteSpan salt,
    const ConstByteSpan blindingFactor) -> ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};
    return signing::signDecoded(key, scheme, message, publicKeyValue, salt, blindingFactor);
}

}
