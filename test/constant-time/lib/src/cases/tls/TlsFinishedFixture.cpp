// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsFinishedFixture.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/KeyAgreementAlgorithm.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/Random.hpp>

namespace app::constant_time {

using namespace el::text::literals;

TlsFinishedFixture::TlsFinishedFixture(
    [[maybe_unused]] el::Random &random, const bool population, el::HashAlgorithm algorithm) {
    using namespace erbsland::cryptology;
    const auto keyBytes = el::ByteBuffer{el::ByteCount{32}, el::Byte{0x55U}};
    const auto key = el::KeyAgreementPrivateKey::fromBytes(el::KeyAgreementAlgorithm::X25519, keyBytes.span());
    auto schedule = std::make_unique<ci::Tls13KeySchedule>(algorithm);
    auto transcriptLength = el::ByteCount{48U};
    if (algorithm == el::HashAlgorithm::Sha2_256) {
        transcriptLength = el::ByteCount{32U};
    }
    auto transcript = el::ByteBuffer{transcriptLength};
    schedule->initializeHandshake(key.agree(key.publicKey()), transcript.span());
    auto expected = el::ByteBuffer{schedule->finishedVerifyData(true, transcript.span()).span()};
    require(schedule->verifyFinished(true, transcript.span(), expected.span()));
    auto index = el::ByteIndex::zero();
    if (population) {
        index = expected.lastIndex();
    }
    expected.set(index, expected.get(index) ^ el::Byte{1U});
    require(!schedule->verifyFinished(true, transcript.span(), expected.span()));
    _schedule = std::move(schedule);
    _transcript = std::move(transcript);
    _expected = std::move(expected);
}

auto TlsFinishedFixture::sample() -> uint64_t {
    return _schedule->verifyFinished(true, _transcript.span(), _expected.span());
}

auto TlsFinishedFixture::measure(const uint64_t repetitions) -> uint64_t {
    return repeat(repetitions, [this]() -> uint64_t { return sample(); });
}

}
