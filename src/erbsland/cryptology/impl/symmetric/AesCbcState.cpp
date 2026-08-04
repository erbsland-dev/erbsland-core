// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesCbcState.hpp"

#include "../algorithm/aes/AesBlockCipherFactory.hpp"

#include "../../../core/Application.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../random/Random.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

using namespace text::literals;

AesCbcState::AesCbcState(
    const SymmetricEncryptionType type, const mem::ConstByteSpan key, const mem::ConstByteSpan iv) :
    _type{type}, _cipher{createAesBlockCipher(key)}, _chain{Block::fromSpanOrThrow(iv)} {
}

AesCbcState::~AesCbcState() noexcept {
    secureErase();
}

auto AesCbcState::encrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    auto output = mem::ByteBlockEditor{};
    output.reserve(unit::ByteLength::fromSizeT(data.size()));
    for (const auto byte : data) {
        _partial.set(unit::ByteIndex{_partialLength}, byte);
        ++_partialLength;
        if (_partialLength == 16U) {
            encryptBlock(_partial, output);
            clearPartial();
        }
    }
    return output;
}

auto AesCbcState::finalizeEncryption() -> mem::ByteBlock {
    auto output = mem::ByteBlockEditor{};
    if (_type == SymmetricEncryptionType::Aes256CbcRandomFill) {
        if (_partialLength == 0U) {
            return output;
        }
        // FFE compatibility: random fill exists only to complete a partial block; aligned input is unchanged.
        const auto missing = 16U - _partialLength;
        auto randomFill = core::application().secureRandom().buildByteBlock(unit::ByteLength{missing});
        _partial.overwrite(unit::ByteIndex{_partialLength}, randomFill.span());
        encryptBlock(_partial, output);
        randomFill.secureErase();
        clearPartial();
        return output;
    }

    // ISO/IEC 9797-1 method 2: append one 0x80 bit followed by zero bits, also for empty/aligned input.
    _partial.set(unit::ByteIndex{_partialLength}, mem::Byte{0x80U});
    encryptBlock(_partial, output);
    clearPartial();
    return output;
}

auto AesCbcState::decrypt(const mem::ConstByteSpan data) -> mem::ByteBlock {
    auto output = mem::ByteBlockEditor{};
    output.reserve(unit::ByteLength::fromSizeT(data.size()));
    for (const auto byte : data) {
        _partial.set(unit::ByteIndex{_partialLength}, byte);
        ++_partialLength;
        if (_partialLength == 16U) {
            acceptCiphertextBlock(_partial, output);
            clearPartial();
        }
    }
    return output;
}

auto AesCbcState::finalizeDecryption() -> mem::ByteBlock {
    if (_partialLength != 0U) {
        throwFinalizationError();
    }
    if (_type == SymmetricEncryptionType::Aes256CbcRandomFill) {
        // FFE random fill is not self-describing. All decrypted bytes, including fill, remain caller-visible.
        return {};
    }
    if (!_hasPendingCiphertext) {
        throwFinalizationError();
    }

    auto paddedPlaintext = _cipher->decrypt(_pendingCiphertext);
    paddedPlaintext ^= _chain;

    // ISO/IEC 9797-1 method 2 validation scans the complete final block without an early exit.
    auto foundMarker = uint8_t{};
    auto invalid = uint8_t{};
    auto markerIndex = std::size_t{};
    for (auto offset = std::size_t{}; offset < 16U; ++offset) {
        const auto index = 15U - offset;
        const auto value = paddedPlaintext.get(unit::ByteIndex{index}).toUInt8();
        const auto isZero = static_cast<uint8_t>(value == 0U);
        const auto isMarker = static_cast<uint8_t>(value == 0x80U);
        const auto unresolved = static_cast<uint8_t>(foundMarker ^ 1U);
        invalid |= static_cast<uint8_t>(unresolved & static_cast<uint8_t>((isZero | isMarker) ^ 1U));
        const auto selectMarker = static_cast<uint8_t>(unresolved & isMarker);
        const auto mask = std::size_t{} - static_cast<std::size_t>(selectMarker);
        markerIndex = (markerIndex & ~mask) | (index & mask);
        foundMarker |= isMarker;
    }
    invalid |= static_cast<uint8_t>(foundMarker ^ 1U);
    if (invalid != 0U) {
        paddedPlaintext.secureErase();
        throwFinalizationError();
    }

    auto output =
        mem::ByteBlockEditor::fromSpan(paddedPlaintext.span(unit::ByteIndex{}, unit::ByteLength{markerIndex}));
    paddedPlaintext.secureErase();
    _pendingCiphertext.secureErase();
    _hasPendingCiphertext = false;
    return output;
}

void AesCbcState::secureErase() noexcept {
    if (_cipher != nullptr) {
        _cipher->secureErase();
        _cipher.reset();
    }
    _chain.secureErase();
    _partial.secureErase();
    _pendingCiphertext.secureErase();
    _partialLength = 0U;
    _hasPendingCiphertext = false;
    _type = {};
}

void AesCbcState::encryptBlock(const Block &plaintext, mem::ByteBlockEditor &output) {
    auto input = plaintext ^ _chain;
    auto ciphertext = _cipher->encrypt(input);
    output.append(ciphertext.span());
    _chain = ciphertext;
    input.secureErase();
    ciphertext.secureErase();
}

void AesCbcState::decryptBlock(const Block &ciphertext, mem::ByteBlockEditor &output) {
    auto plaintext = _cipher->decrypt(ciphertext);
    plaintext ^= _chain;
    output.append(plaintext.span());
    _chain = ciphertext;
    plaintext.secureErase();
}

void AesCbcState::acceptCiphertextBlock(const Block &ciphertext, mem::ByteBlockEditor &output) {
    if (_type == SymmetricEncryptionType::Aes256CbcIso9797Method2) {
        if (_hasPendingCiphertext) {
            decryptBlock(_pendingCiphertext, output);
        }
        _pendingCiphertext = ciphertext;
        _hasPendingCiphertext = true;
        return;
    }
    decryptBlock(ciphertext, output);
}

void AesCbcState::clearPartial() noexcept {
    _partial.secureErase();
    _partialLength = 0U;
}

void AesCbcState::throwFinalizationError() {
    throw CryptologyError{"Invalid AES-CBC ciphertext or padding."_el};
}

}
