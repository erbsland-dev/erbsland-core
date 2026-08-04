// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientProtocol.hpp"

#include "TlsProtocolError.hpp"
#include "TlsWireReader.hpp"
#include "TlsWireWriter.hpp"

#include "../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

auto TlsClientProtocol::sendApplication(const mem::ConstByteSpan data) -> NetworkSendStatus {
    if (_state != TlsClientProtocolState::Established ||
        (hasCheckpoint() && checkpoint() != TlsClientProtocolCheckpoint::HandshakeCompleted) || _localCloseNotify ||
        _peerCloseNotify) {
        return NetworkSendStatus::Closed;
    }
    if (data.empty()) {
        return NetworkSendStatus::Accepted;
    }
    if (data.size() > (std::size_t{1U} << 14U)) {
        throw err::ParameterError{"One TLS application send cannot exceed 2^14 bytes."_el, "data"_el};
    }

    // RFC 8446 sections 5.2 and 5.5: reserve capacity atomically for a possible KeyUpdate plus the complete no-padding
    // application record before either record consumes a sequence number.
    const auto applicationRecordLength = 5U + data.size() + 1U + 16U;
    const auto keyUpdateRecordLength = _encryptor.isKeyUpdateRequired() ? 5U + 5U + 1U + 16U : 0U;
    const auto requiredLength = applicationRecordLength + keyUpdateRecordLength;
    const auto limit = _options.bufferLimits().send().toSizeT();
    if (requiredLength > limit || _transportOutputBytes > limit - requiredLength) {
        return NetworkSendStatus::WouldBlock;
    }

    if (_encryptor.isKeyUpdateRequired()) {
        // RFC 8446 section 4.6.3: protect KeyUpdate with the old write keys, then install the new generation before the
        // following application record. update_requested=0 avoids reciprocal-update loops.
        auto keyUpdate = TlsWireWriter{};
        keyUpdate.writeU8(24U);
        keyUpdate.writeU24(1U);
        keyUpdate.writeU8(0U);
        const auto keyUpdateMessage = keyUpdate.finish();
        queueProtectedHandshake(keyUpdateMessage.span());
        _encryptor.updateApplicationTrafficKeys();
    }

    // RFC 8446 section 5.2: application bytes become visible to transport only in an authenticated application_data
    // TLSInnerPlaintext. The record layer increments the sequence exactly once after successful protection.
    auto record = _encryptor.protect(cryptology::TlsRecordContentType::ApplicationData, data);
    if (!queueTransport(std::move(record))) {
        fail(TlsAlertDescription::InternalError, "A preflighted TLS application record was not queued."_el, false);
        return NetworkSendStatus::Closed;
    }
    return NetworkSendStatus::Accepted;
}

void TlsClientProtocol::processPostHandshake(const mem::ByteBlock &message) {
    const auto type = message.getOrThrow(unit::ByteIndex::zero()).toUInt8();
    if (type == 4U) {
        // RFC 8446 section 4.6.1: parse NewSessionTicket strictly, but discard it because PSK resumption is deferred.
        auto reader = TlsWireReader{message.span().subspan(4U)};
        [[maybe_unused]] const auto ticketLifetime = reader.readU32();
        [[maybe_unused]] const auto ticketAgeAdd = reader.readU32();
        [[maybe_unused]] const auto ticketNonce = reader.readVector8();
        const auto ticket = reader.readVector16();
        if (ticket.empty()) {
            throw TlsProtocolError{TlsAlertDescription::DecodeError, "NewSessionTicket contains an empty ticket."_el};
        }
        const auto extensionBytes = reader.readVector16();
        reader.requireEnd();
        forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
            if (extension.type != 42U) {
                throw TlsProtocolError{
                    TlsAlertDescription::UnsupportedExtension,
                    "NewSessionTicket contains an unsupported extension."_el};
            }
            auto extensionReader = TlsWireReader{extension.data};
            [[maybe_unused]] const auto maximumEarlyDataSize = extensionReader.readU32();
            extensionReader.requireEnd();
        });
        return;
    }
    if (type == 24U) {
        auto reader = TlsWireReader{message.span().subspan(4U)};
        const auto request = reader.readU8();
        reader.requireEnd();
        if (request > 1U) {
            throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "KeyUpdate request value is invalid."_el};
        }

        // RFC 8446 section 5.1: KeyUpdate immediately precedes a key change and must align with the record boundary.
        if (!_handshakeStream.bufferedLength().isZero()) {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the KeyUpdate key boundary."_el};
        }

        // RFC 8446 section 4.6.3: the received KeyUpdate was authenticated with old read keys; install the new read
        // generation before accepting the next peer record.
        _decryptor.updateApplicationTrafficKeys();
        if (request == 1U) {
            // RFC 8446 section 4.6.3: send the requested response before the next application record using the old
            // write keys, then advance the client write generation.
            auto response = TlsWireWriter{};
            response.writeU8(24U);
            response.writeU24(1U);
            response.writeU8(0U);
            const auto responseMessage = response.finish();
            queueProtectedHandshake(responseMessage.span());
            _encryptor.updateApplicationTrafficKeys();
        }
        return;
    }

    // RFC 8446 section 4.6.2: post-handshake client authentication is not implemented and therefore fails closed.
    throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Unsupported TLS post-handshake message."_el};
}

void TlsClientProtocol::close() {
    if (_state == TlsClientProtocolState::Inactive || _state == TlsClientProtocolState::Closed ||
        _state == TlsClientProtocolState::Failed) {
        return;
    }
    if (!_localCloseNotify) {
        queueAlert(TlsAlertDescription::CloseNotify);
    }
    if (_localCloseNotify) {
        _state = _peerCloseNotify && _transportOutput.empty() ? TlsClientProtocolState::Closed
                                                              : TlsClientProtocolState::Closing;
    }
}

void TlsClientProtocol::queueAlert(const TlsAlertDescription description) noexcept {
    try {
        const auto level = description == TlsAlertDescription::CloseNotify ? uint8_t{1U} : uint8_t{2U};
        const auto content = mem::ByteArray<2U>{mem::Byte{level}, mem::Byte{static_cast<uint8_t>(description)}};

        if (!_encryptor.isEmpty()) {
            // RFC 8446 section 6: alerts are exactly one authenticated alert record under the current write keys.
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

        // RFC 8446 sections 5.1 and 6: before handshake keys exist, serialize one plaintext fatal/warning alert.
        auto record = TlsWireWriter{};
        record.writeU8(21U);
        record.writeU16(0x0303U);
        record.writeU16(2U);
        record.writeBytes(content.span());
        if (queueTransport(record.finish()) && description == TlsAlertDescription::CloseNotify) {
            _localCloseNotify = true;
        }
    } catch (...) {
        // Alert transmission is best effort on a terminal or closing path; no exception may mask the original failure.
    }
}

void TlsClientProtocol::fail(const TlsAlertDescription alert, text::String diagnostic, const bool sendAlert) noexcept {
    if (_state == TlsClientProtocolState::Failed || _state == TlsClientProtocolState::Closed) {
        return;
    }
    if (sendAlert) {
        queueAlert(alert);
    }
    _failureAlert = alert;
    _failureDiagnostic = std::move(diagnostic);
    eraseSecurityState();
    _state = TlsClientProtocolState::Failed;
}

void TlsClientProtocol::eraseSecurityState() noexcept {
    // Keep every terminal secret release visible in protocol ownership order: ECDHE, schedule, transcript-dependent
    // state, active record generations, then authenticated plaintext queued for the application.
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
    for (auto &block : _applicationInput) {
        block.secureErase();
    }
    _applicationInput.clear();
    _applicationInputBytes = 0U;
    _checkpoint = TlsClientProtocolCheckpoint::None;
    _serverPublicKey = {};
}

}
