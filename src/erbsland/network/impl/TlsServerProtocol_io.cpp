// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "TlsProtocolError.hpp"
#include "TlsWireReader.hpp"

#include "../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../cryptology/tls_record/TlsRecordError.hpp"
#include "../../cryptology/tls_record/TlsRecordErrorCategory.hpp"
#include "../../err/Exception.hpp"
#include "../../mem/Byte.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerProtocol::feedTransport(const mem::ConstByteSpan data) noexcept {
    if (_state == TlsServerProtocolState::Inactive || _state == TlsServerProtocolState::Closed ||
        _state == TlsServerProtocolState::Failed) {
        return;
    }
    try {
        requireReceiveCapacity(data.size());
        _recordStream.append(data);
        if (!hasCheckpoint()) {
            processRetainedInput();
        }
    } catch (const TlsProtocolError &error) {
        fail(error.alert(), error.reason());
    } catch (const cryptology::TlsRecordError &error) {
        auto alert = TlsAlertDescription::InternalError;
        switch (error.category()) {
        case cryptology::TlsRecordErrorCategory::DecodeError:
            alert = TlsAlertDescription::DecodeError;
            break;
        case cryptology::TlsRecordErrorCategory::UnexpectedMessage:
            alert = TlsAlertDescription::UnexpectedMessage;
            break;
        case cryptology::TlsRecordErrorCategory::BadRecordMac:
            alert = TlsAlertDescription::BadRecordMac;
            break;
        case cryptology::TlsRecordErrorCategory::RecordOverflow:
            alert = TlsAlertDescription::RecordOverflow;
            break;
        case cryptology::TlsRecordErrorCategory::KeyUsageExhausted:
            alert = TlsAlertDescription::InternalError;
            break;
        }
        fail(alert, error.reason());
    } catch (const err::Exception &error) {
        fail(TlsAlertDescription::InternalError, error.reason());
    } catch (...) {
        fail(TlsAlertDescription::InternalError, "An unexpected TLS server protocol operation failed."_el);
    }
}

void TlsServerProtocol::processRetainedInput() {
    processHandshakeMessages();
    if (!hasCheckpoint()) {
        processRecords();
    }
}

void TlsServerProtocol::processRetainedInputNoThrow() noexcept {
    try {
        processRetainedInput();
    } catch (const TlsProtocolError &error) {
        fail(error.alert(), error.reason());
    } catch (const cryptology::TlsRecordError &error) {
        auto alert = TlsAlertDescription::InternalError;
        switch (error.category()) {
        case cryptology::TlsRecordErrorCategory::DecodeError:
            alert = TlsAlertDescription::DecodeError;
            break;
        case cryptology::TlsRecordErrorCategory::UnexpectedMessage:
            alert = TlsAlertDescription::UnexpectedMessage;
            break;
        case cryptology::TlsRecordErrorCategory::BadRecordMac:
            alert = TlsAlertDescription::BadRecordMac;
            break;
        case cryptology::TlsRecordErrorCategory::RecordOverflow:
            alert = TlsAlertDescription::RecordOverflow;
            break;
        case cryptology::TlsRecordErrorCategory::KeyUsageExhausted:
            alert = TlsAlertDescription::InternalError;
            break;
        }
        fail(alert, error.reason());
    } catch (const err::Exception &error) {
        fail(TlsAlertDescription::InternalError, error.reason());
    } catch (...) {
        fail(TlsAlertDescription::InternalError, "An unexpected TLS server protocol operation failed."_el);
    }
}

void TlsServerProtocol::requireReceiveCapacity(const std::size_t additionalBytes) const {
    const auto limit = _options.bufferLimits().receive().toSizeT();
    const auto retained = _recordStream.bufferedLength().toSizeT() + _handshakeStream.bufferedLength().toSizeT() +
        _clientHello.length().toSizeT() + _applicationInputBytes;
    if (additionalBytes > limit || retained > limit - additionalBytes) {
        throw TlsProtocolError{TlsAlertDescription::RecordOverflow, "The aggregate TLS receive limit was exceeded."_el};
    }
}

void TlsServerProtocol::processRecords() {
    while (const auto record = _recordStream.next()) {
        if (_state == TlsServerProtocolState::Failed || _state == TlsServerProtocolState::Closed) {
            return;
        }
        if (_state == TlsServerProtocolState::Handshaking) {
            ++_handshakeRecordCount;
            // RFC 8446 Section 4: cap handshake record work independently from byte allocation bounds.
            if (_handshakeRecordCount > cMaximumHandshakeRecords) {
                throw TlsProtocolError{
                    TlsAlertDescription::UnexpectedMessage, "The TLS handshake record limit was exceeded."_el};
            }
        }
        processRecord(*record);
        if (hasCheckpoint()) {
            return;
        }
    }
}

void TlsServerProtocol::processRecord(const mem::ByteBlock &record) {
    auto reader = TlsWireReader{record.span()};
    const auto type = reader.readU8();
    const auto legacyVersion = reader.readU16();
    const auto content = reader.readBytes(reader.readU16());
    reader.requireEnd();

    // RFC 8446 Appendix D.2: servers ignore the initial ClientHello record-layer version; after negotiation every
    // TLSPlaintext and TLSCiphertext legacy_record_version is 0x0303.
    if (!(_handshakeStep == HandshakeStep::ClientHello && type == 22U) && legacyVersion != 0x0303U) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "TLS record legacy version is invalid."_el};
    }

    // RFC 8446 Section 5.1 and Appendix D.4: tolerate an exact dummy CCS only between ClientHello and peer Finished.
    if (type == 20U) {
        if (!_handshakeStream.bufferedLength().isZero() || _state != TlsServerProtocolState::Handshaking ||
            _handshakeStep != HandshakeStep::ClientFinished || content.size() != 1U || content[0] != mem::Byte{1U}) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "TLS ChangeCipherSpec is invalid."_el};
        }
        ++_ignoredCcsCount;
        if (_ignoredCcsCount > cMaximumIgnoredCcs) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "TLS ChangeCipherSpec tolerance limit was exceeded."_el};
        }
        return;
    }

    if (_decryptor.isEmpty()) {
        processPlaintextRecord(type, content);
        return;
    }
    processProtectedRecord(record);
}

void TlsServerProtocol::processPlaintextRecord(const uint8_t type, const mem::ConstByteSpan content) {
    // RFC 8446 Section 5.1: no non-handshake content may interleave a fragmented handshake message.
    if (type != 22U && !_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "A record interleaves a fragmented TLS handshake message."_el};
    }
    switch (type) {
    case 21U:
        processAlert(content);
        return;
    case 22U:
        if (content.empty()) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "A zero-length TLS handshake fragment was received."_el};
        }
        _handshakeStream.append(content);
        processHandshakeMessages();
        return;
    default:
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Unexpected plaintext before TLS server authentication."_el};
    }
}

void TlsServerProtocol::processProtectedRecord(const mem::ByteBlock &record) {
    // RFC 8446 Section 5.2: no inner content is inspected or released until AEAD authentication succeeds.
    auto plaintext = _decryptor.unprotect(record.span());
    if (plaintext.type() != cryptology::TlsRecordContentType::Handshake &&
        !_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "A record interleaves a fragmented TLS handshake message."_el};
    }
    switch (plaintext.type()) {
    case cryptology::TlsRecordContentType::Alert:
        processAlert(plaintext.content().span());
        break;
    case cryptology::TlsRecordContentType::Handshake:
        if (plaintext.content().isEmpty()) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "A zero-length TLS handshake fragment was received."_el};
        }
        _handshakeStream.append(plaintext.content().span());
        processHandshakeMessages();
        break;
    case cryptology::TlsRecordContentType::ApplicationData:
        if (_handshakeStep != HandshakeStep::PostHandshake) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "TLS application data arrived before client Finished."_el};
        }
        processApplicationData(plaintext.content().span());
        break;
    default:
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Unsupported authenticated TLS content type."_el};
    }
}

void TlsServerProtocol::processHandshakeMessages() {
    if (hasCheckpoint()) {
        return;
    }
    while (const auto message = _handshakeStream.next()) {
        processHandshake(*message);
        if (hasCheckpoint()) {
            return;
        }
    }
}

}
