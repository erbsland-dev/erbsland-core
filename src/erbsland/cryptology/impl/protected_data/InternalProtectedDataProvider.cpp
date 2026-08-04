// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InternalProtectedDataProvider.hpp"

#include "../../../core/Application.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../random/Random.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../CryptologyError.hpp"
#include "../../symmetric/SymmetricDecryptor.hpp"
#include "../../symmetric/SymmetricEncryptionType.hpp"
#include "../../symmetric/SymmetricEncryptor.hpp"
#include "../../symmetric/SymmetricTag.hpp"

#include <limits>

namespace erbsland::cryptology::impl {

using namespace text::literals;

InternalProtectedDataProvider::InternalProtectedDataProvider() :
    _key{core::application().secureRandom().buildByteBlock(unit::ByteLength{32U})} {
    // NIST SP 800-38D section 8: this uniformly random AES-256 key belongs to exactly one application-lifetime
    // invocation-counter domain. SymmetricKey retains it in sensitive storage.
}

InternalProtectedDataProvider::~InternalProtectedDataProvider() {
    // End the key's lifetime explicitly while provider storage is still valid; SymmetricKey erases its old bytes.
    _key = {};
}

auto InternalProtectedDataProvider::protect(const mem::ConstByteSpan plaintext, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // NIST SP 800-38D sections 5.2.1 and 8.2.1: serialize all uses of the key so each invocation gets a unique IV.
    const auto lock = std::scoped_lock{_mutex};
    const auto nonce = nextNonce();

    // NIST SP 800-38D section 7.1: authenticate the external plaintext length as AAD before encrypting P.
    auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256Gcm, _key, nonce};
    encryptor.addAuthenticatedData(lengthData(plaintextLength));
    const auto encrypted = encryptor.encrypt(plaintext);
    const auto finalData = encryptor.finalize();
    const auto tag = encryptor.tag();

    // Provider envelope format: 96-bit IV || 128-bit authentication tag || GCM ciphertext. This format is private and
    // has no persistence or cross-provider compatibility contract.
    auto envelope = mem::ByteBuffer{};
    envelope.reserve(
        unit::ByteLength{cNonceSize + cTagSize + encrypted.length().toSizeT() + finalData.length().toSizeT()});
    envelope.append(nonce.span());
    envelope.append(tag.span());
    envelope.append(encrypted.span());
    envelope.append(finalData.span());
    auto result = mem::ByteBlock::fromSpan(envelope.span());
    result.markAsSensitive();
    return result;
}

auto InternalProtectedDataProvider::unprotect(const mem::ConstByteSpan envelope, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // Validate the private envelope framing before creating any cryptographic state or slicing caller-controlled data.
    const auto expectedLength = cNonceSize + cTagSize + plaintextLength.toSizeT();
    if (envelope.size() != expectedLength) {
        throw CryptologyError{"Invalid protected-data envelope."_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    const auto nonce = SymmetricNonce{envelope.first(cNonceSize)};
    const auto tag = SymmetricTag{envelope.subspan(cNonceSize, cTagSize)};
    const auto encrypted = envelope.subspan(cNonceSize + cTagSize);

    // NIST SP 800-38D section 7.2: supply the same AAD and withhold the provisional plaintext from the caller until
    // finalize verifies the 128-bit tag. SymmetricDecryptor marks both plaintext fragments as sensitive immediately.
    auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256Gcm, _key, nonce};
    decryptor.addAuthenticatedData(lengthData(plaintextLength));
    auto plaintext = decryptor.decrypt(encrypted);
    auto finalData = decryptor.finalize(tag);
    if (!finalData.isEmpty()) {
        // The combined temporary remains sensitive, so replacement and every exceptional exit erase its allocation.
        auto combined = mem::ByteBuffer{plaintext.span()};
        combined.setSensitive(true);
        combined.append(finalData.span());
        plaintext = mem::ByteBlock::fromSpan(combined.span());
    }
    plaintext.markAsSensitive();

    // The GCM tag authenticates plaintextLength through AAD; retain a defensive postcondition before releasing P.
    if (plaintext.length() != plaintextLength) {
        plaintext.secureErase();
        throw CryptologyError{"Protected-data length authentication failed."_el};
    }
    return plaintext;
}

auto InternalProtectedDataProvider::lengthData(const unit::ByteLength plaintextLength) -> mem::ByteBlock {
    // Provider metadata encoding: a fixed-width big-endian integer avoids ambiguous AAD encodings of the same length.
    auto data = mem::ByteArray<8U>{};
    data.setIntegerOrThrow<uint64_t>(
        unit::ByteIndex::zero(), static_cast<uint64_t>(plaintextLength.toSizeT()), mem::Endianness::Big);
    return mem::ByteBlock{data};
}

auto InternalProtectedDataProvider::nextNonce() -> SymmetricNonce {
    // NIST SP 800-38D section 8.2.1 deterministic construction: reject exhaustion before a counter can wrap and reuse
    // an IV under this key. Zero is reserved as the pre-invocation state and is never emitted.
    if (_nonceCounter == std::numeric_limits<uint64_t>::max()) {
        throw CryptologyError{"The protected-data nonce space is exhausted."_el};
    }
    ++_nonceCounter;

    // The 96-bit IV is a 32-bit zero fixed field followed by the 64-bit big-endian invocation field. This provider is
    // the sole invoker for its fresh application key, so key-scoped uniqueness comes entirely from the counter.
    auto nonce = mem::ByteArray<cNonceSize>{};
    nonce.setIntegerOrThrow<uint64_t>(unit::ByteIndex{4U}, _nonceCounter, mem::Endianness::Big);
    return SymmetricNonce{nonce.span()};
}

}
