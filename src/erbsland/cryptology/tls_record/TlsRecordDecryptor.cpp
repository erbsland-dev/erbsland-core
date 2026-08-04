// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordDecryptor.hpp"

#include "TlsRecordError.hpp"

#include "../CryptologyError.hpp"
#include "../impl/SecureEraseGuard.hpp"
#include "../impl/tls/TlsRecordState.hpp"
#include "../symmetric/SymmetricDecryptor.hpp"
#include "../symmetric/SymmetricTag.hpp"

#include "../../err/LogicError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/Literals.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

TlsRecordDecryptor::TlsRecordDecryptor() noexcept = default;

TlsRecordDecryptor::TlsRecordDecryptor(const TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret) :
    _state{std::make_unique<impl::TlsRecordState>(suite, std::move(trafficSecret))} {
    // The consumed traffic secret is now owned only by protected per-direction state and is erased with that state.
}

TlsRecordDecryptor::~TlsRecordDecryptor() {
    secureErase();
}

TlsRecordDecryptor::TlsRecordDecryptor(TlsRecordDecryptor &&) noexcept = default;

auto TlsRecordDecryptor::operator=(TlsRecordDecryptor &&) noexcept -> TlsRecordDecryptor & = default;

auto TlsRecordDecryptor::unprotect(const mem::ConstByteSpan record) -> TlsRecordPlaintext {
    static constexpr std::size_t cHeaderLength{5U};
    static constexpr std::size_t cMaximumContentLength{uint16_t{1U} << 14U};
    static constexpr std::size_t cMaximumInnerPlaintextLength{cMaximumContentLength + 1U};
    static constexpr std::size_t cAuthenticationTagLength{16U};
    static constexpr std::size_t cMaximumCiphertextLength{cMaximumContentLength + 256U};

    if (_state == nullptr) {
        throw err::LogicError{"The TLS record decryptor is empty."_el};
    }

    try {
        // RFC 8446 section 5.2: one TLSCiphertext begins with an exact five-byte outer record header.
        if (record.size() < cHeaderLength) {
            throw TlsRecordError{TlsRecordErrorCategory::DecodeError, "The TLS record header is truncated."_el};
        }

        // RFC 8446 section 5.2: protected records use opaque_type application_data(23) and version 0x0303.
        if (record[0].toUInt8() != 23U || record[1].toUInt8() != 0x03U || record[2].toUInt8() != 0x03U) {
            throw TlsRecordError{
                TlsRecordErrorCategory::UnexpectedMessage,
                "The TLSCiphertext outer type or legacy version is invalid."_el};
        }

        // RFC 8446 section 5.2: length is a network-order uint16 and the caller must provide exactly that one record.
        const auto declaredLength = (static_cast<std::size_t>(record[3].toUInt8()) << 8U) | record[4].toUInt8();
        if (declaredLength > cMaximumCiphertextLength) {
            throw TlsRecordError{
                TlsRecordErrorCategory::RecordOverflow,
                "TLSCiphertext exceeds the RFC 8446 limit of 2^14 + 256 bytes."_el};
        }
        if (record.size() != cHeaderLength + declaredLength) {
            throw TlsRecordError{
                TlsRecordErrorCategory::DecodeError, "The TLS record length does not match the supplied bytes."_el};
        }
        if (declaredLength < cAuthenticationTagLength + 1U ||
            declaredLength > cMaximumInnerPlaintextLength + cAuthenticationTagLength) {
            throw TlsRecordError{
                declaredLength > cMaximumInnerPlaintextLength + cAuthenticationTagLength
                    ? TlsRecordErrorCategory::RecordOverflow
                    : TlsRecordErrorCategory::DecodeError,
                "The TLS encrypted record body has an invalid length."_el};
        }

        _state->requireRecordAvailable();

        const auto encryptedLength = declaredLength - cAuthenticationTagLength;
        const auto header = record.first(cHeaderLength);
        const auto encrypted = record.subspan(cHeaderLength, encryptedLength);
        const auto receivedTag = SymmetricTag{record.last(cAuthenticationTagLength)};

        // RFC 8446 section 5.3: derive the per-record nonce from write_iv and the current sequence number.
        auto nonce = _state->recordNonce();

        // RFC 8446 section 5.2: additional_data is the exact received five-byte TLSCiphertext header.
        auto decryptor = SymmetricDecryptor{_state->suite().encryptionType(), _state->key(), nonce};
        decryptor.addAuthenticatedData(header);

        // RFC 8446 section 5.2: decrypt with write_key, nonce, and additional_data, but do not expose this tentative
        // TLSInnerPlaintext until AEAD authentication succeeds below.
        auto tentativePlaintext = decryptor.decrypt(encrypted);
        tentativePlaintext.markAsSensitive();
        const auto tentativePlaintextEraseGuard = impl::SecureEraseGuard{tentativePlaintext};
        try {
            // Keep any final authenticated plaintext fragment in sensitive guarded storage until it is transferred to
            // the combined tentative TLSInnerPlaintext allocation.
            auto finalPlaintext = decryptor.finalize(receivedTag);
            finalPlaintext.markAsSensitive();
            const auto finalPlaintextEraseGuard = impl::SecureEraseGuard{finalPlaintext};
            tentativePlaintext = mem::ByteBlock{mem::ByteBlockEditor{tentativePlaintext}.append(finalPlaintext.span())};
            tentativePlaintext.markAsSensitive();
        } catch (const CryptologyError &) {
            // Authentication rejected the tentative plaintext; its guard erases it before this categorized failure
            // exits.
            throw TlsRecordError{TlsRecordErrorCategory::BadRecordMac, "TLS record authentication failed."_el};
        }

        // RFC 8446 section 5.4: scan TLSInnerPlaintext backwards across zero padding to the first nonzero type octet.
        auto typeIndex = tentativePlaintext.span().size();
        while (typeIndex > 0U && tentativePlaintext.span()[typeIndex - 1U] == mem::Byte{}) {
            --typeIndex;
        }
        if (typeIndex == 0U) {
            throw TlsRecordError{
                TlsRecordErrorCategory::UnexpectedMessage, "TLSInnerPlaintext contains no nonzero content type."_el};
        }

        // RFC 8446 section 5.4: the first nonzero octet from the end is TLSInnerPlaintext.type.
        const auto rawType = tentativePlaintext.span()[typeIndex - 1U].toUInt8();
        auto type = TlsRecordContentType::ApplicationData;
        switch (rawType) {
        case static_cast<uint8_t>(TlsRecordContentType::Alert):
            type = TlsRecordContentType::Alert;
            break;
        case static_cast<uint8_t>(TlsRecordContentType::Handshake):
            type = TlsRecordContentType::Handshake;
            break;
        case static_cast<uint8_t>(TlsRecordContentType::ApplicationData):
            type = TlsRecordContentType::ApplicationData;
            break;
        default:
            throw TlsRecordError{
                TlsRecordErrorCategory::UnexpectedMessage,
                "TLSInnerPlaintext contains an unsupported content type."_el};
        }

        const auto contentLength = typeIndex - 1U;
        if (contentLength > cMaximumContentLength) {
            throw TlsRecordError{
                TlsRecordErrorCategory::RecordOverflow, "TLSInnerPlaintext content exceeds 2^14 bytes."_el};
        }
        if (contentLength == 0U && type != TlsRecordContentType::ApplicationData) {
            throw TlsRecordError{
                TlsRecordErrorCategory::UnexpectedMessage,
                "Empty TLSInnerPlaintext is permitted only for application data."_el};
        }

        // Copy authenticated content into its own sensitive allocation before erasing type and padding scratch bytes.
        auto content = mem::ByteBlock::fromSpan(tentativePlaintext.span().first(contentLength));
        content.markAsSensitive();

        // RFC 8446 section 5.3: advance the receiving sequence exactly once after authentication and validation.
        _state->recordSucceeded();

        // Ownership of authenticated plaintext transfers to the result; its destructor or caller erases the allocation.
        return TlsRecordPlaintext{type, std::move(content)};
    } catch (...) {
        // RFC-facing peer rejection and backend failure are terminal for this direction; erase secret, key, IV, and
        // counters.
        secureErase();
        throw;
    }
}

void TlsRecordDecryptor::updateApplicationTrafficKeys() {
    if (_state == nullptr) {
        throw err::LogicError{"The TLS record decryptor is empty."_el};
    }
    try {
        // RFC 8446 sections 4.6.3 and 7.2: replace this receiving direction after an authenticated KeyUpdate message.
        _state->updateApplicationTrafficKeys();
    } catch (...) {
        // Failed replacement is terminal: erase retained old state and any incomplete next generation.
        secureErase();
        throw;
    }
}

void TlsRecordDecryptor::secureErase() noexcept {
    if (_state != nullptr) {
        _state->secureErase();
        _state.reset();
    }
}

auto TlsRecordDecryptor::isKeyUpdateRequired() const noexcept -> bool {
    return _state != nullptr && _state->isKeyUpdateRequired();
}

auto TlsRecordDecryptor::suite() const -> TlsCipherSuite {
    if (_state == nullptr) {
        throw err::LogicError{"The TLS record decryptor is empty."_el};
    }
    return _state->suite();
}

}
