// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>

namespace demo {

/// Generate an Ed25519 signing key and distribute its public verifier separately.
///
/// A signed score record can be checked by readers who have the public key. Keep the private PKCS#8 key with the
/// score publisher; the public SubjectPublicKeyInfo key may be shared with every verifier. PEM conversion here models
/// separate storage without writing a private key or its text to the console.
void createKeys() {
    // Generate this pair once for the publisher and export the two roles separately.
    auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
    const auto privatePem = signingKey.toPem();
    const auto publicPem = signingKey.publicKey().toPem();

    // Each side imports only the key it needs.
    const auto publisherKey = el::cryptology::SigningPrivateKey::fromPemOrThrow(privatePem);
    const auto readerKey = el::cryptology::PublicKey::fromPemOrThrow(publicPem);
    el::io::printLine("Publisher and reader keys match: "_el, publisherKey.matches(readerKey));
}

}
