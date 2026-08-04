// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesGcmState.hpp"

#include "../algorithm/aes/AesBlockCipherFactory.hpp"
#include "../algorithm/aes/GaloisMultiplierFactory.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

AesGcmState::AesGcmState(const mem::ConstByteSpan key, const mem::ConstByteSpan nonce) :
    _cipher{createAesBlockCipher(key)} {
    // SP 800-38D, Section 7.1: for a 96-bit IV, J0 is IV followed by 31 zero bits and one bit.
    _initialCounter.overwrite(nonce);
    _initialCounter.setIntegerOrThrow(unit::ByteIndex{12U}, uint32_t{1U}, mem::Endianness::Big);
    _counter = _initialCounter;

    // SP 800-38D, Section 5.3: H is the AES encryption of the all-zero block.
    auto hashSubkey = _cipher->encrypt(Block{});
    _gHash = std::make_unique<GHash>(hashSubkey, createGaloisMultiplier());
    hashSubkey.secureErase();
}

AesGcmState::~AesGcmState() noexcept {
    secureErase();
}

void AesGcmState::addAuthenticatedData(const mem::ConstByteSpan data) {
    if (data.size() > maximumAuthenticatedDataLength - _authenticatedDataLength) {
        throw CryptologyError{"AES-GCM authenticated data exceeds the per-message limit."_el};
    }
    _gHash->addAuthenticatedData(data);
    _authenticatedDataLength += static_cast<uint64_t>(data.size());
}

auto AesGcmState::transform(const mem::ConstByteSpan data, const bool encrypting) -> mem::ByteBlock {
    if (data.size() > maximumPayloadLength - _payloadLength) {
        throw CryptologyError{"AES-GCM payload exceeds the per-message limit."_el};
    }

    // SP 800-38D, Sections 6.5 and 7: GCTR XORs input with successive encrypted inc32 counter blocks.
    auto result = mem::ByteBlockEditor{unit::ByteLength::fromSizeT(data.size())};
    for (auto index = std::size_t{}; index < data.size(); ++index) {
        if (_keyStreamOffset == 16U) {
            generateKeyStreamBlock();
        }
        result.set(unit::ByteIndex{index}, data[index] ^ _keyStream.get(unit::ByteIndex{_keyStreamOffset}));
        ++_keyStreamOffset;
    }

    // SP 800-38D, Sections 7.1 and 7.2: GHASH always authenticates ciphertext, never plaintext.
    _gHash->addCiphertext(encrypting ? result.span() : data);
    _payloadLength += static_cast<uint64_t>(data.size());
    return result;
}

auto AesGcmState::finalizeTag() noexcept -> Block {
    // SP 800-38D, Equation 4: T = GCTR_K(J0, GHASH(A, C)); for a full tag this is AES(J0) XOR GHASH.
    auto authentication = _gHash->finalize();
    auto result = _cipher->encrypt(_initialCounter);
    result ^= authentication;
    authentication.secureErase();
    return result;
}

void AesGcmState::secureErase() noexcept {
    if (_gHash != nullptr) {
        _gHash->secureErase();
        _gHash.reset();
    }
    if (_cipher != nullptr) {
        _cipher->secureErase();
        _cipher.reset();
    }
    _initialCounter.secureErase();
    _counter.secureErase();
    _keyStream.secureErase();
    _keyStreamOffset = 16U;
    _authenticatedDataLength = 0U;
    _payloadLength = 0U;
}

void AesGcmState::incrementCounter() noexcept {
    const auto value = _counter.getInteger<uint32_t>(unit::ByteIndex{12U}, mem::Endianness::Big);
    _counter.setIntegerOrThrow(unit::ByteIndex{12U}, value + 1U, mem::Endianness::Big);
}

void AesGcmState::generateKeyStreamBlock() noexcept {
    incrementCounter();
    _keyStream = _cipher->encrypt(_counter);
    _keyStreamOffset = 0U;
}

}
