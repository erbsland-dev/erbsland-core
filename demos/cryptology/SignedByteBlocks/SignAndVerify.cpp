// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/SignedByteBlock.hpp>
#include <erbsland/text/StringEncoder.hpp>

namespace demo {

/// Sign a complete score record with a purpose reserved for this record type.
///
/// The `SignedByteBlock` stores the exact payload bytes, a purpose, an optional key lookup hint, and an Ed25519
/// signature. The serialized block is self-contained and can be stored or sent without the private key.
void signAndVerify() {
    const auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
    const auto scoreText = el::String{"Poäng: 42; nivå: skog"_el};
    const auto score = el::StringEncoder{scoreText}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);

    // Bind the score bytes to this application use and include a public lookup hint.
    const auto signedScore =
        el::cryptology::SignedByteBlock::sign(signingKey, score.span(), "puzzle.score/v1"_el, "score_2026"_el);
    const auto storedRecord = signedScore.toByteBlock();
    el::io::printLine("Serialized signed score bytes: "_el, storedRecord.length());
    el::io::printLine(
        "Publisher verifies its copy: "_el,
        signedScore.verify(signingKey.publicKey(), "puzzle.score/v1"_el).has_value());
}

}
