// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TlsFinishedFixture.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

TlsFinishedFixture::TlsFinishedFixture(
    [[maybe_unused]] el::Random &random, const bool population, el::HashAlgorithm algorithm) {
    using namespace erbsland::cryptology;
    const auto keyBytes = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x55U}};
    const auto key = el::KeyAgreementPrivateKey::fromBytes(el::KeyAgreementAlgorithm::X25519, keyBytes.span());
    auto schedule = std::make_unique<ci::Tls13KeySchedule>(algorithm);
    auto transcript = el::ByteBuffer{el::ByteLength{algorithm == el::HashAlgorithm::Sha2_256 ? 32U : 48U}};
    schedule->initializeHandshake(key.agree(key.publicKey()), transcript.span());
    auto expected = el::ByteBuffer{schedule->finishedVerifyData(true, transcript.span()).span()};
    require(schedule->verifyFinished(true, transcript.span(), expected.span()));
    const auto index = population ? expected.length().toSizeT() - 1 : 0;
    expected.set(el::ByteIndex{index}, expected.get(el::ByteIndex{index}) ^ el::Byte{1U});
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
