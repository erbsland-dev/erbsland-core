// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChaCha20Poly1305State.hpp"

#include "../algorithm/chacha20/ChaCha20BackendFactory.hpp"
#include "../algorithm/chacha20/Poly1305Factory.hpp"

#include "../../../err/RuntimeError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace unit;
using namespace text::literals;

ChaCha20Poly1305State::ChaCha20Poly1305State(const ConstByteSpan key, const ConstByteSpan nonce) :
    _chacha20{createChaCha20Backend(key, nonce)} {
    // RFC 8439, Sections 2.6 and 2.8: counter zero produces the first 256 bits of the one-time Poly1305 key.
    auto firstBlock = _chacha20->generateBlocks(0U, 1U);
    auto oneTimeKey = ByteArray<32>{};
    oneTimeKey.overwrite(firstBlock.span(ByteIndex{0U}, ByteCount{32U}));
    try {
        _poly1305 = createPoly1305(oneTimeKey.span());
    } catch (const err::RuntimeError &) {
        oneTimeKey.secureErase();
        firstBlock.secureErase();
        _chacha20->secureErase();
        throw;
    }
    oneTimeKey.secureErase();
    firstBlock.secureErase();
}

ChaCha20Poly1305State::~ChaCha20Poly1305State() noexcept {
    secureErase();
}

void ChaCha20Poly1305State::addAuthenticatedData(const ConstByteSpan data) {
    // RFC 8439, Section 2.8: the final framing stores the AAD byte count in one unsigned 64-bit field.
    if (ByteCount::fromSizeT(data.size()) > ByteCount::maximum() - _authenticatedDataLength) {
        throw CryptologyError{"ChaCha20-Poly1305 authenticated data exceeds the per-message limit."_el};
    }
    _poly1305->update(data);
    _authenticatedDataLength += ByteCount::fromSizeT(data.size());
}

auto ChaCha20Poly1305State::transform(const ConstByteSpan data, const bool encrypting) -> ByteBlock {
    if (data.empty()) {
        // An empty payload call does not end the AAD phase; the public facade permits more AAD afterwards.
        return {};
    }
    if (ByteCount::fromSizeT(data.size()) > ByteCount{maximumPayloadLength} - _payloadLength) {
        throw CryptologyError{"ChaCha20-Poly1305 payload exceeds the RFC 8439 counter limit."_el};
    }
    finishAuthenticatedData();

    // RFC 8439, Sections 2.4 and 2.8: payload counters start at one and XOR the input with the key stream.
    auto result = ByteBlockEditor{ByteCount::fromSizeT(data.size())};
    for (auto index = std::size_t{}; index < data.size(); ++index) {
        if (_keyStreamOffset == ByteIndex::end(_keyStreamLength)) {
            generateKeyStream(data.size() - index);
        }
        result.set(ByteIndex{index, ByteIndex::Unchecked{}}, data[index] ^ _keyStream.get(_keyStreamOffset));
        _keyStreamOffset.uncheckedIncrement();
    }

    // RFC 8439, Section 2.8: Poly1305 authenticates ciphertext for both encryption and decryption.
    _poly1305->update(encrypting ? result.span() : data);
    _payloadLength += ByteCount::fromSizeT(data.size());
    return result;
}

auto ChaCha20Poly1305State::finalizeTag() noexcept -> Tag {
    finishAuthenticatedData();

    // RFC 8439, Section 2.8: ciphertext is followed by pad16(ciphertext).
    addPadding(_payloadLength);

    // RFC 8439, Section 2.8: append the AAD and ciphertext byte lengths as two little-endian uint64 values.
    auto lengths = ByteArray<16>{};
    lengths.setIntegerOrThrow(ByteIndex{0U}, _authenticatedDataLength.toRawValue(), Endianness::Little);
    lengths.setIntegerOrThrow(ByteIndex{8U}, _payloadLength.toRawValue(), Endianness::Little);
    _poly1305->update(lengths.span());
    auto result = _poly1305->finalize();
    lengths.secureErase();
    return result;
}

void ChaCha20Poly1305State::secureErase() noexcept {
    if (_poly1305 != nullptr) {
        _poly1305->secureErase();
        _poly1305.reset();
    }
    if (_chacha20 != nullptr) {
        _chacha20->secureErase();
        _chacha20.reset();
    }
    _keyStream.secureErase();
    _keyStreamOffset = ByteIndex{0U};
    _keyStreamLength = ByteCount{0U};
    _nextCounter = 0U;
    _authenticatedDataLength = ByteCount{0U};
    _payloadLength = ByteCount{0U};
    _authenticatedDataFinished = false;
}

void ChaCha20Poly1305State::finishAuthenticatedData() noexcept {
    if (_authenticatedDataFinished) {
        return;
    }
    // RFC 8439, Section 2.8: pad the AAD before the first non-empty ciphertext byte or finalization.
    addPadding(_authenticatedDataLength);
    _authenticatedDataFinished = true;
}

void ChaCha20Poly1305State::addPadding(const ByteCount length) noexcept {
    const auto paddingLength = (ByteCount{16U} - (length % 16U)) % 16U;
    if (paddingLength == ByteCount{0U}) {
        return;
    }
    const auto zeroes = ByteArray<16>{};
    _poly1305->update(zeroes.span(ByteIndex{0U}, paddingLength));
}

void ChaCha20Poly1305State::generateKeyStream(const std::size_t remainingInput) noexcept {
    // RFC 8439, Sections 2.4 and 2.8: map a four-block batch to consecutive payload counters without wraparound.
    // Four consecutive counters are submitted only when every resulting byte is immediately consumed.
    const auto canGenerateBatch = remainingInput >= 256U && _nextCounter + 3U <= 0xffffffffU;
    const auto blockCount = canGenerateBatch ? std::size_t{4U} : std::size_t{1U};
    _keyStream.secureErase();
    _keyStream = _chacha20->generateBlocks(static_cast<uint32_t>(_nextCounter), blockCount);
    _keyStreamOffset = ByteIndex{0U};
    _keyStreamLength = ByteCount::fromSizeT(blockCount * 64U);
    _nextCounter += blockCount;
}

}
