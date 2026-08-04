// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordState.hpp"

#include "Tls13Hkdf.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../mem/Byte.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../Hkdf.hpp"
#include "../../tls_record/TlsRecordError.hpp"

#include <limits>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace text::literals;

TlsRecordState::TlsRecordState(const TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret) :
    _suite{suite}, _trafficSecret{std::move(trafficSecret)} {
    if (!TlsCipherSuite::fromRawValue(_suite.toRawValue()).has_value()) {
        throw err::ParameterError{"The TLS cipher suite is not supported."_el, "suite"_el};
    }
    if (_trafficSecret.isEmpty() || _trafficSecret.hashAlgorithm() != _suite.hashAlgorithm()) {
        throw err::ParameterError{
            "The TLS traffic secret does not match the cipher suite hash."_el, "trafficSecret"_el};
    }
    // RFC 8446 section 7.3: derive write_key and write_iv before the first record uses sequence number zero.
    deriveKeyAndIv(_trafficSecret, _key, _staticIv);
}

TlsRecordState::~TlsRecordState() {
    secureErase();
}

auto TlsRecordState::recordNonce() const -> SymmetricNonce {
    // RFC 8446 section 5.3 step 1: encode the 64-bit sequence in network order and left-pad it to iv_length.
    auto paddedSequence = mem::ByteBlockEditor{unit::ByteLength{12U}};
    paddedSequence.markAsSensitive();
    paddedSequence.setIntegerOrThrow<uint64_t>(unit::ByteIndex{4U}, _sequenceNumber, mem::Endianness::Big);

    // RFC 8446 section 5.3 step 2: nonce = write_iv XOR padded_sequence_number.
    auto nonce = mem::ByteBlockEditor{_staticIv.data()};
    nonce.xorWithOrThrow(paddedSequence.span());
    paddedSequence.secureErase();
    return SymmetricNonce{mem::ByteBlock{nonce}};
}

void TlsRecordState::requireRecordAvailable() const {
    if (_suite.encryptionType().cipher() == SymmetricCipher::Aes) {
        // RFC 8446 section 5.5: refuse AES-GCM at the floor of the 2^24.5-record confidentiality limit.
        if (_recordCount >= cAesMaximumRecordCount) {
            throw TlsRecordError{
                TlsRecordErrorCategory::KeyUsageExhausted, "The TLS AES-GCM record limit is exhausted."_el};
        }
        return;
    }
    // RFC 8446 sections 5.3 and 5.5: do not process a record whose post-success increment would wrap 64 bits.
    if (_sequenceNumber == std::numeric_limits<uint64_t>::max()) {
        throw TlsRecordError{
            TlsRecordErrorCategory::KeyUsageExhausted, "The TLS record sequence number is exhausted."_el};
    }
}

void TlsRecordState::recordSucceeded() noexcept {
    ++_sequenceNumber;
    ++_recordCount;
}

void TlsRecordState::updateApplicationTrafficKeys() {
    // RFC 8446 section 7.2: derive application_traffic_secret_N+1 before replacing any old-generation state.
    auto nextSecret = deriveUpdatedTrafficSecret();
    auto nextKey = SymmetricKey{};
    auto nextIv = SymmetricNonce{};
    deriveKeyAndIv(nextSecret, nextKey, nextIv);

    // Preserve ownership of the old generation while the completely derived replacement is installed with noexcept
    // moves. This keeps the RFC 8446 section 7.2 transition atomic from the state object's perspective.
    auto oldSecret = std::move(_trafficSecret);
    auto oldKey = std::move(_key);
    auto oldStaticIv = std::move(_staticIv);
    _trafficSecret = std::move(nextSecret);
    _key = std::move(nextKey);
    _staticIv = std::move(nextIv);
    _sequenceNumber = 0U;
    _recordCount = 0U;

    // RFC 8446 section 7.2: immediately erase the preceding traffic secret, write_key, and write_iv after replacement.
    oldSecret.secureErase();
    oldKey = {};
    oldStaticIv = {};
}

void TlsRecordState::secureErase() noexcept {
    // Keep every retained secret release visible: protected secret envelope first, then active key and IV allocations.
    _trafficSecret.secureErase();
    _key = {};
    _staticIv = {};
    _sequenceNumber = 0U;
    _recordCount = 0U;
}

auto TlsRecordState::isKeyUpdateRequired() const noexcept -> bool {
    if (_suite.encryptionType().cipher() == SymmetricCipher::Aes) {
        return _recordCount >= cAesUpdateRecordCount;
    }
    // Leave one record available for the old-key KeyUpdate handshake message itself.
    return _sequenceNumber >= std::numeric_limits<uint64_t>::max() - 1U;
}

void TlsRecordState::deriveKeyAndIv(const TlsTrafficSecret &secret, SymmetricKey &key, SymmetricNonce &staticIv) const {
    static constexpr auto cKeyLabel = mem::ByteArray<3U>{mem::Byte{'k'}, mem::Byte{'e'}, mem::Byte{'y'}};
    static constexpr auto cIvLabel = mem::ByteArray<2U>{mem::Byte{'i'}, mem::Byte{'v'}};
    const auto tlsHkdf = Tls13Hkdf{_suite.hashAlgorithm()};

    // Resolve Secret only for the two RFC 8446 section 7.3 expansions; ProtectedByteBlock erases on every exit.
    secret._data.withUnprotectedData([&](const mem::ConstByteSpan secretBytes) -> void {
        // RFC 8446 section 7.3: write_key = HKDF-Expand-Label(Secret, "key", "", key_length).
        auto keyBytes = tlsHkdf.expandLabel(secretBytes, cKeyLabel.span(), {}, _suite.encryptionType().keyLength());
        keyBytes.markAsSensitive();
        const auto keyEraseGuard = SecureEraseGuard{keyBytes};
        key = SymmetricKey{keyBytes.span()};

        // RFC 8446 section 7.3: write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length).
        auto ivBytes = tlsHkdf.expandLabel(secretBytes, cIvLabel.span(), {}, unit::ByteLength{12U});
        ivBytes.markAsSensitive();
        const auto ivEraseGuard = SecureEraseGuard{ivBytes};
        staticIv = SymmetricNonce{ivBytes.span()};
    });
}

auto TlsRecordState::deriveUpdatedTrafficSecret() const -> TlsTrafficSecret {
    static constexpr auto cTrafficUpdateLabel = mem::ByteArray<11U>{
        mem::Byte{'t'},
        mem::Byte{'r'},
        mem::Byte{'a'},
        mem::Byte{'f'},
        mem::Byte{'f'},
        mem::Byte{'i'},
        mem::Byte{'c'},
        mem::Byte{' '},
        mem::Byte{'u'},
        mem::Byte{'p'},
        mem::Byte{'d'}};
    auto result = TlsTrafficSecret{};
    const auto tlsHkdf = Tls13Hkdf{_suite.hashAlgorithm()};

    // Resolve application_traffic_secret_N only within protected storage's erased callback temporary.
    _trafficSecret._data.withUnprotectedData([&](const mem::ConstByteSpan secretBytes) -> void {
        // RFC 8446 section 7.2: application_traffic_secret_N+1 = HKDF-Expand-Label(N, "traffic upd", "", Hash.length).
        auto nextBytes =
            tlsHkdf.expandLabel(secretBytes, cTrafficUpdateLabel.span(), {}, _suite.hashAlgorithm().digestSize());
        nextBytes.markAsSensitive();
        // The consuming factory protects the next secret and erases this plaintext allocation before returning.
        result = TlsTrafficSecret::fromBytes(_suite.hashAlgorithm(), std::move(nextBytes));
    });
    return result;
}

}
