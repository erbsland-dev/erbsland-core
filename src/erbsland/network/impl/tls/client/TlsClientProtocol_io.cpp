// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/CryptologyError.hpp"
#include "../../../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../../../cryptology/tls_record/TlsRecordError.hpp"
#include "../../../../cryptology/tls_record/TlsRecordErrorCategory.hpp"
#include "../../../../err/Exception.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteLength.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsClientProtocol::feedTransport(const mem::ConstByteSpan data) noexcept {
    if (_state == TlsClientProtocolState::Inactive || _state == TlsClientProtocolState::Closed ||
        _state == TlsClientProtocolState::Failed) {
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
        fail(TlsAlertDescription::InternalError, "An unexpected TLS protocol operation failed."_el);
    }
}

void TlsClientProtocol::resume() noexcept {
    if (!hasCheckpoint() || _state == TlsClientProtocolState::Closed || _state == TlsClientProtocolState::Failed) {
        return;
    }
    _checkpoint = TlsClientProtocolCheckpoint::None;
    processRetainedInputNoThrow();
}

void TlsClientProtocol::processRetainedInput() {
    processHandshakeMessages();
    if (!hasCheckpoint()) {
        processRecords();
    }
}

void TlsClientProtocol::processRetainedInputNoThrow() noexcept {
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
        fail(TlsAlertDescription::InternalError, "An unexpected TLS protocol operation failed."_el);
    }
}

void TlsClientProtocol::requireReceiveCapacity(const std::size_t additionalBytes) const {
    const auto limit = _options.bufferLimits().receive().toSizeT();
    const auto retained =
        _recordStream.bufferedLength().toSizeT() + _handshakeStream.bufferedLength().toSizeT() + _applicationInputBytes;
    if (additionalBytes > limit || retained > limit - additionalBytes) {
        throw TlsProtocolError{TlsAlertDescription::RecordOverflow, "The aggregate TLS receive limit was exceeded."_el};
    }
}

void TlsClientProtocol::processRecords() {
    while (const auto record = _recordStream.next()) {
        if (_state == TlsClientProtocolState::Failed || _state == TlsClientProtocolState::Closed) {
            return;
        }
        if (_state == TlsClientProtocolState::Handshaking) {
            ++_handshakeRecordCount;
            // This fixed work bound prevents an attacker from sustaining a fragmented handshake indefinitely even
            // when every individual record and message remains under its allocation bound.
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

void TlsClientProtocol::processRecord(const mem::ByteBlock &record) {
    auto reader = TlsWireReader{record.span()};
    const auto type = reader.readU8();
    const auto legacyVersion = reader.readU16();
    const auto content = reader.readBytes(reader.readU16());
    reader.requireEnd();

    // RFC 8446 section 5.1 and appendix D.4: peer records use legacy_record_version 0x0303 after ClientHello.
    if (legacyVersion != 0x0303U) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "The TLS record legacy version is invalid."_el};
    }

    // RFC 8446 appendix D.4: tolerate an exact dummy CCS only during the handshake, even after keys are installed.
    if (type == 20U) {
        if (!_handshakeStream.bufferedLength().isZero()) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "ChangeCipherSpec interleaves a fragmented handshake."_el};
        }
        if (_state != TlsClientProtocolState::Handshaking || _handshakeStep == HandshakeStep::PostHandshake ||
            content.size() != 1U || content[0] != mem::Byte{1U}) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "The TLS ChangeCipherSpec is invalid."_el};
        }
        ++_ignoredCcsCount;
        if (_ignoredCcsCount > cMaximumIgnoredCcs) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "The TLS ChangeCipherSpec tolerance limit was exceeded."_el};
        }
        return;
    }

    if (_decryptor.isEmpty()) {
        processPlaintextRecord(type, content);
        return;
    }
    processProtectedRecord(record);
}

void TlsClientProtocol::processPlaintextRecord(const uint8_t type, const mem::ConstByteSpan content) {
    // RFC 8446 section 5.1: no other record type may interleave a fragmented handshake message.
    if (type != 22U && !_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "A record interleaves a fragmented TLS handshake message."_el};
    }
    switch (type) {
    case 21U:
        processAlert(content);
        return;
    case 22U:
        // RFC 8446 section 5.1: plaintext handshake fragments may contain partial or multiple handshake messages.
        if (content.empty()) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "A zero-length TLS handshake fragment was received."_el};
        }
        _handshakeStream.append(content);
        processHandshakeMessages();
        return;
    default:
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "An unexpected plaintext TLS record type was received."_el};
    }
}

void TlsClientProtocol::processProtectedRecord(const mem::ByteBlock &record) {
    // RFC 8446 section 5.2: no inner content is inspected or released until AEAD authentication succeeds.
    auto plaintext = _decryptor.unprotect(record.span());
    // RFC 8446 section 5.1: an incomplete handshake message cannot be interleaved with another inner content type.
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
                TlsAlertDescription::UnexpectedMessage, "TLS application data arrived before server Finished."_el};
        }
        processApplicationData(plaintext.content().span());
        break;
    default:
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "An unsupported authenticated TLS content type was received."_el};
    }
}

void TlsClientProtocol::processHandshakeMessages() {
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

void TlsClientProtocol::processAlert(const mem::ConstByteSpan content) {
    // RFC 8446 section 6: every alert record contains exactly AlertLevel and AlertDescription.
    if (content.size() != 2U) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "A TLS alert must contain exactly two bytes."_el};
    }
    const auto level = content[0].toUInt8();
    const auto description = content[1].toUInt8();
    if (level != 1U && level != 2U) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "The TLS alert level is invalid."_el};
    }
    if (description == static_cast<uint8_t>(TlsAlertDescription::CloseNotify)) {
        // RFC 8446 section 6.1: close_notify ends peer writes; later application records are ignored.
        _peerCloseNotify = true;
        if (!_localCloseNotify) {
            queueAlert(TlsAlertDescription::CloseNotify);
            _localCloseNotify = true;
        }
        _state = _transportOutput.empty() ? TlsClientProtocolState::Closed : TlsClientProtocolState::Closing;
        return;
    }
    if (level == 1U && description == static_cast<uint8_t>(TlsAlertDescription::UserCanceled)) {
        // RFC 8446 section 6.1 permits user_canceled at warning level and requires recipients to ignore it. JSSE
        // emits this warning immediately before close_notify when closing a connection after application traffic.
        return;
    }
    _failureWasPeerAlert = true;
    fail(static_cast<TlsAlertDescription>(description), "The TLS peer reported a fatal alert."_el, false);
}

void TlsClientProtocol::processApplicationData(const mem::ConstByteSpan content) {
    if (_peerCloseNotify) {
        return;
    }
    if (content.empty()) {
        return;
    }
    requireReceiveCapacity(content.size());
    auto block = mem::ByteBlock::fromSpan(content);
    block.markAsSensitive();
    _applicationInputBytes += block.length().toSizeT();
    _applicationInput.push_back(std::move(block));
}

void TlsClientProtocol::timeout() noexcept {
    if (_state == TlsClientProtocolState::Handshaking) {
        // A local deadline is not a peer protocol error and must not attempt a possibly blocked transport write.
        fail(TlsAlertDescription::InternalError, "The TLS handshake deadline expired."_el, false);
    }
}

void TlsClientProtocol::transportClosed() noexcept {
    if (_state == TlsClientProtocolState::Closed || _state == TlsClientProtocolState::Failed) {
        return;
    }
    if (_peerCloseNotify) {
        eraseSecurityState();
        _state = TlsClientProtocolState::Closed;
        return;
    }
    _failureWasTruncation = true;
    fail(TlsAlertDescription::UnexpectedMessage, "The TLS transport closed without close_notify."_el, false);
}

void TlsClientProtocol::abort() noexcept {
    eraseSecurityState();
    _transportOutput.clear();
    _transportOutputBytes = 0U;
    _checkpoint = TlsClientProtocolCheckpoint::None;
    _state = TlsClientProtocolState::Closed;
}

}
