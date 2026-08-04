// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordEncryptor.hpp"

#include "../impl/SecureEraseGuard.hpp"
#include "../impl/tls/TlsRecordState.hpp"
#include "../symmetric/SymmetricEncryptor.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/Endianness.hpp"
#include "../../text/Literals.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

TlsRecordEncryptor::TlsRecordEncryptor() noexcept = default;

TlsRecordEncryptor::TlsRecordEncryptor(const TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret) :
    _state{std::make_unique<impl::TlsRecordState>(suite, std::move(trafficSecret))} {
    // The consumed traffic secret is now owned only by protected per-direction state and is erased with that state.
}

TlsRecordEncryptor::~TlsRecordEncryptor() {
    secureErase();
}

TlsRecordEncryptor::TlsRecordEncryptor(TlsRecordEncryptor &&) noexcept = default;

auto TlsRecordEncryptor::operator=(TlsRecordEncryptor &&) noexcept -> TlsRecordEncryptor & = default;

auto TlsRecordEncryptor::protect(
    const TlsRecordContentType type, const mem::ConstByteSpan content, const unit::ByteLength paddingLength)
    -> mem::ByteBlock {
    static constexpr std::size_t cMaximumContentLength{uint16_t{1U} << 14U};
    static constexpr std::size_t cMaximumInnerPlaintextLength{cMaximumContentLength + 1U};
    static constexpr std::size_t cAuthenticationTagLength{16U};

    if (_state == nullptr) {
        throw err::LogicError{"The TLS record encryptor is empty."_el};
    }
    if (type != TlsRecordContentType::Alert && type != TlsRecordContentType::Handshake &&
        type != TlsRecordContentType::ApplicationData) {
        throw err::ParameterError{"Unsupported TLS record content type."_el, "type"_el};
    }
    if (content.empty() && type != TlsRecordContentType::ApplicationData) {
        throw err::ParameterError{"Empty TLS records are permitted only for application data."_el, "content"_el};
    }
    if (content.size() > cMaximumContentLength ||
        paddingLength.toSizeT() > cMaximumInnerPlaintextLength - 1U - content.size()) {
        throw err::ParameterError{"TLSInnerPlaintext exceeds the RFC 8446 limit of 2^14 + 1 bytes."_el, "content"_el};
    }

    try {
        _state->requireRecordAvailable();

        // RFC 8446 sections 5.2 and 5.4: TLSInnerPlaintext.content = the caller's authenticated content bytes.
        auto innerPlaintext = mem::ByteBlockEditor::fromSpan(content);
        innerPlaintext.markAsSensitive();
        const auto innerPlaintextEraseGuard = impl::SecureEraseGuard{innerPlaintext};

        // RFC 8446 section 5.2: TLSInnerPlaintext.type is the nonzero content-type octet after content.
        innerPlaintext.append(mem::Byte{static_cast<uint8_t>(type)});

        // RFC 8446 section 5.4: zeros[length_of_padding] is explicit caller-selected padding; no policy is applied
        // here.
        innerPlaintext.append(mem::Byte{}, paddingLength);

        // RFC 8446 section 5.2: opaque_type is application_data(23), legacy_record_version is 0x0303, and length
        // covers the encrypted TLSInnerPlaintext plus the fixed 128-bit authentication tag.
        const auto encryptedLength = innerPlaintext.span().size() + cAuthenticationTagLength;
        auto header = mem::ByteBlockEditor{};
        header.append(mem::Byte{23U});
        header.appendInteger<uint16_t>(0x0303U, mem::Endianness::Big);
        header.appendInteger<uint16_t>(static_cast<uint16_t>(encryptedLength), mem::Endianness::Big);

        // RFC 8446 section 5.3: derive the per-record nonce from write_iv and the current sequence number.
        auto nonce = _state->recordNonce();

        // RFC 8446 section 5.2: additional_data is the exact encoded five-byte TLSCiphertext header.
        auto encryptor = SymmetricEncryptor{_state->suite().encryptionType(), _state->key(), nonce};
        encryptor.addAuthenticatedData(header.span());

        // RFC 8446 section 5.2: AEADEncrypted = AEAD-Encrypt(write_key, nonce, additional_data, TLSInnerPlaintext).
        auto encrypted = encryptor.encrypt(innerPlaintext.span());
        auto finalEncrypted = encryptor.finalize();
        const auto tag = encryptor.tag();

        // RFC 8446 section 5.2: emit exactly one TLSCiphertext header followed by ciphertext and the 128-bit tag.
        auto record = mem::ByteBlockEditor{};
        record.append(header.span());
        record.append(encrypted.span());
        record.append(finalEncrypted.span());
        record.append(tag.span());

        // RFC 8446 section 5.3: increment the sequence exactly once, only after the complete record exists.
        _state->recordSucceeded();
        return mem::ByteBlock{record};
    } catch (...) {
        // Once key use or backend processing begins, erase the generation so an uncertain nonce is never reused.
        secureErase();
        throw;
    }
}

void TlsRecordEncryptor::updateApplicationTrafficKeys() {
    if (_state == nullptr) {
        throw err::LogicError{"The TLS record encryptor is empty."_el};
    }
    try {
        // RFC 8446 sections 4.6.3 and 7.2: replace this sending direction at the caller-directed KeyUpdate point.
        _state->updateApplicationTrafficKeys();
    } catch (...) {
        // Failed replacement is terminal: erase both retained old state and any partially built backend state.
        secureErase();
        throw;
    }
}

void TlsRecordEncryptor::secureErase() noexcept {
    if (_state != nullptr) {
        _state->secureErase();
        _state.reset();
    }
}

auto TlsRecordEncryptor::isKeyUpdateRequired() const noexcept -> bool {
    return _state != nullptr && _state->isKeyUpdateRequired();
}

auto TlsRecordEncryptor::suite() const -> TlsCipherSuite {
    if (_state == nullptr) {
        throw err::LogicError{"The TLS record encryptor is empty."_el};
    }
    return _state->suite();
}

}
