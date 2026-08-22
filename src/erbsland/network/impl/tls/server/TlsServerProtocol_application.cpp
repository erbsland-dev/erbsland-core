// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../../../err/Exception.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteArray.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

auto TlsServerProtocol::sendApplication(const mem::ConstByteSpan data) -> NetworkSendStatus {
    if (_state != TlsServerProtocolState::Established || hasCheckpoint() || _localCloseNotify || _peerCloseNotify) {
        return NetworkSendStatus::Closed;
    }
    if (data.size() > (std::size_t{1U} << 14U)) {
        throw err::ParameterError{"A TLS application send exceeds 2^14 bytes."_el, "data"_el};
    }
    if (data.empty()) {
        return NetworkSendStatus::Accepted;
    }
    // RFC 8446 Sections 5.2 and 5.5: preflight a possible KeyUpdate plus the exact no-padding application record so
    // WouldBlock never consumes a record sequence number or partially transfers caller ownership.
    const auto applicationRecordLength = 5U + data.size() + 1U + 16U;
    const auto keyUpdateRecordLength = _encryptor.isKeyUpdateRequired() ? 5U + 5U + 1U + 16U : 0U;
    const auto requiredLength = applicationRecordLength + keyUpdateRecordLength;
    const auto limit = _options.bufferLimits().send().toSizeT();
    if (requiredLength > limit || _transportOutputBytes > limit - requiredLength) {
        return NetworkSendStatus::WouldBlock;
    }
    try {
        if (_encryptor.isKeyUpdateRequired()) {
            // RFC 8446 Sections 4.6.3 and 7.2: protect KeyUpdate(request_update=0) with the old server write
            // generation, then install server_application_traffic_secret_N+1 before the application record.
            auto keyUpdate = TlsWireWriter{};
            keyUpdate.writeU8(24U);
            keyUpdate.writeU24(1U);
            keyUpdate.writeU8(0U);
            const auto keyUpdateMessage = keyUpdate.finish();
            queueProtectedHandshake(keyUpdateMessage.span());
            _encryptor.updateApplicationTrafficKeys();
        }

        // RFC 8446 Section 5.2: application bytes become visible to transport only as authenticated
        // application_data TLSInnerPlaintext under the current server write generation.
        auto record = _encryptor.protect(cryptology::TlsRecordContentType::ApplicationData, data);
        if (!queueTransport(std::move(record))) {
            fail(TlsAlertDescription::InternalError, "A preflighted TLS application record was not queued."_el, false);
            return NetworkSendStatus::Closed;
        }
    } catch (const err::Exception &error) {
        fail(TlsAlertDescription::InternalError, error.reason());
        return NetworkSendStatus::Closed;
    }
    return NetworkSendStatus::Accepted;
}

void TlsServerProtocol::processPostHandshake(const mem::ByteBlock &message) {
    const auto type = message.getOrThrow(unit::ByteIndex::zero()).toUInt8();
    if (type != 24U) {
        // RFC 8446 Sections 4.6.1--4.6.2: clients never send NewSessionTicket and this profile does not request or
        // permit post-handshake authentication or renegotiation.
        throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Unsupported client post-handshake message."_el};
    }

    auto reader = TlsWireReader{message.span().subspan(4U)};
    const auto request = reader.readU8();
    reader.requireEnd();
    if (request > 1U) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "KeyUpdate request value is invalid."_el};
    }
    // RFC 8446 Section 5.1: KeyUpdate immediately precedes a key change and therefore ends at a record boundary.
    if (!_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the KeyUpdate boundary."_el};
    }

    // RFC 8446 Sections 4.6.3 and 7.2: the received KeyUpdate used the old client read generation; install the next
    // generation before accepting another client record.
    _decryptor.updateApplicationTrafficKeys();
    if (request == 1U) {
        // RFC 8446 Section 4.6.3: protect the response with the old server write generation, then advance that write
        // generation before any later application or alert record.
        auto response = TlsWireWriter{};
        response.writeU8(24U);
        response.writeU24(1U);
        response.writeU8(0U);
        const auto responseMessage = response.finish();
        queueProtectedHandshake(responseMessage.span());
        _encryptor.updateApplicationTrafficKeys();
    }
}

void TlsServerProtocol::processAlert(const mem::ConstByteSpan content) {
    // RFC 8446 Section 6: every alert record contains exactly AlertLevel and AlertDescription.
    if (content.size() != 2U) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "A TLS alert must contain exactly two bytes."_el};
    }
    const auto level = content[0].toUInt8();
    const auto description = content[1].toUInt8();
    if (level != 1U && level != 2U) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "TLS alert level is invalid."_el};
    }
    if (description == static_cast<uint8_t>(TlsAlertDescription::CloseNotify)) {
        // RFC 8446 Section 6.1: close_notify ends peer writes. Reply once and close after accepted output drains.
        _peerCloseNotify = true;
        if (!_localCloseNotify) {
            queueAlert(TlsAlertDescription::CloseNotify);
        }
        if (_transportOutput.empty()) {
            eraseSecurityState();
            _state = TlsServerProtocolState::Closed;
        } else {
            _state = TlsServerProtocolState::Closing;
        }
        return;
    }
    if (level == 1U && description == static_cast<uint8_t>(TlsAlertDescription::UserCanceled)) {
        return;
    }
    _failureWasPeerAlert = true;
    fail(static_cast<TlsAlertDescription>(description), "TLS client reported a fatal alert."_el, false);
}

void TlsServerProtocol::processApplicationData(const mem::ConstByteSpan content) {
    if (_peerCloseNotify || content.empty()) {
        return;
    }
    requireReceiveCapacity(content.size());
    auto block = mem::ByteBlock::fromSpan(content);
    block.markAsSensitive();
    _applicationInputBytes += block.length().toSizeT();
    _applicationInput.push_back(std::move(block));
}

void TlsServerProtocol::close() {
    if (_state == TlsServerProtocolState::Inactive || _state == TlsServerProtocolState::Closed ||
        _state == TlsServerProtocolState::Failed) {
        return;
    }
    if (!_localCloseNotify) {
        queueAlert(TlsAlertDescription::CloseNotify);
    }
    if (_localCloseNotify) {
        _state = _peerCloseNotify && _transportOutput.empty() ? TlsServerProtocolState::Closed
                                                              : TlsServerProtocolState::Closing;
    }
}

void TlsServerProtocol::queueProtectedHandshake(const mem::ConstByteSpan message) {
    const auto recordLength = 5U + message.size() + 1U + 16U;
    const auto limit = _options.bufferLimits().send().toSizeT();
    if (recordLength > limit || _transportOutputBytes > limit - recordLength) {
        throw TlsProtocolError{
            TlsAlertDescription::InternalError, "TLS output queue cannot hold a handshake record."_el};
    }
    auto record = _encryptor.protect(cryptology::TlsRecordContentType::Handshake, message);
    if (!queueTransport(std::move(record))) {
        throw TlsProtocolError{TlsAlertDescription::InternalError, "A preflighted handshake record was not queued."_el};
    }
}

void TlsServerProtocol::queueAlert(const TlsAlertDescription description) noexcept {
    try {
        const auto level = description == TlsAlertDescription::CloseNotify ? uint8_t{1U} : uint8_t{2U};
        const auto content = mem::ByteArray<2U>{mem::Byte{level}, mem::Byte{static_cast<uint8_t>(description)}};
        if (!_encryptor.isEmpty()) {
            constexpr auto cProtectedAlertRecordLength = std::size_t{5U + 2U + 1U + 16U};
            const auto limit = _options.bufferLimits().send().toSizeT();
            if (cProtectedAlertRecordLength > limit || _transportOutputBytes > limit - cProtectedAlertRecordLength) {
                return;
            }
            auto record = _encryptor.protect(cryptology::TlsRecordContentType::Alert, content.span());
            if (queueTransport(std::move(record)) && description == TlsAlertDescription::CloseNotify) {
                _localCloseNotify = true;
            }
            return;
        }

        // RFC 8446 Sections 5.1 and 6: before server handshake keys exist, send one plaintext alert with the legacy
        // TLS 1.2 record version. Alert transmission is best effort on terminal paths.
        auto record = TlsWireWriter{};
        record.writeU8(21U);
        record.writeU16(0x0303U);
        record.writeU16(2U);
        record.writeBytes(content.span());
        if (queueTransport(record.finish()) && description == TlsAlertDescription::CloseNotify) {
            _localCloseNotify = true;
        }
    } catch (...) {
        // A best-effort alert must never replace the original failure or escape a noexcept terminal path.
    }
}

void TlsServerProtocol::fail(const TlsAlertDescription alert, text::String diagnostic, const bool sendAlert) noexcept {
    if (_state == TlsServerProtocolState::Failed || _state == TlsServerProtocolState::Closed) {
        return;
    }
    if (sendAlert) {
        queueAlert(alert);
    }
    _failureAlert = alert;
    _failureDiagnostic = std::move(diagnostic);
    eraseSecurityState();
    _state = TlsServerProtocolState::Failed;
}

void TlsServerProtocol::eraseSecurityState() noexcept {
    // Keep every secret release visible in ownership order: ephemeral X25519, schedule, transcript-dependent state,
    // active record generations, deframed sensitive bytes, then authenticated plaintext queued for the application.
    _privateKey.secureErase();
    if (_keySchedule != nullptr) {
        _keySchedule->secureErase();
        _keySchedule.reset();
    }
    if (_transcript != nullptr) {
        _transcript->secureErase();
        _transcript.reset();
    }
    _encryptor.secureErase();
    _decryptor.secureErase();
    _handshakeStream.clear();
    _recordStream.clear();
    _clientHello = {};
    _legacySessionId = {};
    _clientKey = {};
    for (auto &block : _applicationInput) {
        block.secureErase();
    }
    _applicationInput.clear();
    _applicationInputBytes = 0U;
    _checkpoint = TlsServerProtocolCheckpoint::None;
}

void TlsServerProtocol::timeout() noexcept {
    if (_state == TlsServerProtocolState::Handshaking) {
        fail(TlsAlertDescription::InternalError, "TLS server handshake deadline expired."_el, false);
    }
}

void TlsServerProtocol::transportClosed() noexcept {
    if (_state == TlsServerProtocolState::Closed || _state == TlsServerProtocolState::Failed) {
        return;
    }
    if (_peerCloseNotify) {
        eraseSecurityState();
        _state = TlsServerProtocolState::Closed;
        return;
    }
    _failureWasTruncation = true;
    fail(TlsAlertDescription::UnexpectedMessage, "TLS transport closed without close_notify."_el, false);
}

void TlsServerProtocol::abort() noexcept {
    if (_state == TlsServerProtocolState::Closed || _state == TlsServerProtocolState::Failed) {
        return;
    }
    eraseSecurityState();
    _transportOutput.clear();
    _transportOutputBytes = 0U;
    _checkpoint = TlsServerProtocolCheckpoint::None;
    _state = TlsServerProtocolState::Closed;
}

}
