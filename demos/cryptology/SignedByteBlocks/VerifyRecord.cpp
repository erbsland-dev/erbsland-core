// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/SignedByteBlock.hpp>
#include <erbsland/text/StringEncoder.hpp>

namespace demo {

/// Parse an untrusted signed record under a payload limit, then verify before using its contents.
///
/// Parsing only checks the binary format. The key hint is available for finding a candidate public key, but neither
/// it nor the payload is trusted until `verify()` succeeds with the application's expected purpose.
void verifyRecord() {
    const auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
    const auto scoreText = el::String{"Poäng: 42; nivå: skog"_el};
    const auto score = el::StringEncoder{scoreText}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
    const auto receivedBytes =
        el::cryptology::SignedByteBlock::sign(signingKey, score.span(), "puzzle.score/v1"_el, "score_2026"_el)
            .toByteBlock();

    // Treat bytes from storage or a peer as untrusted, even if the record parses.
    const auto record =
        el::cryptology::SignedByteBlock::fromByteBlockOrThrow(receivedBytes.span(), el::ByteLength{256U});
    // The hint selects a candidate from a trusted local key registry; it is not proof by itself.
    if (!record.untrustedKeyIdHint().has_value() || *record.untrustedKeyIdHint() != "score_2026"_el) {
        throw el::ApplicationError{"Unknown score publisher key."_el};
    }
    const auto candidateKey = signingKey.publicKey();
    const auto verifiedScore = record.verify(candidateKey, "puzzle.score/v1"_el);
    el::io::printLine("Verified score matches source: "_el, verifiedScore.has_value() && *verifiedScore == score);
    el::io::printLine("Wrong purpose accepted: "_el, record.verify(candidateKey, "puzzle.level/v1"_el).has_value());
}

}
