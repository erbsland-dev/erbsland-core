// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "TlsProtocolError.hpp"

#include "../../core/Application.hpp"
#include "../../err/Exception.hpp"
#include "../../err/LogicError.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

TlsServerProtocol::TlsServerProtocol(TlsServerProtocolOptions options) :
    _options{std::move(options)}, _recordStream{_options.bufferLimits().receive()} {
}

TlsServerProtocol::~TlsServerProtocol() {
    eraseSecurityState();
}

void TlsServerProtocol::start() {
    if (_state != TlsServerProtocolState::Inactive) {
        throw err::LogicError{"The TLS server protocol has already been started."_el};
    }
    // RFC 8446 Appendix A.2, START -> REC: enter the receive state without generating server secrets or output.
    _state = TlsServerProtocolState::Handshaking;
}

void TlsServerProtocol::resume() noexcept {
    if (!hasCheckpoint() || _state == TlsServerProtocolState::Closed || _state == TlsServerProtocolState::Failed) {
        return;
    }
    const auto checkpoint = _checkpoint;
    _checkpoint = TlsServerProtocolCheckpoint::None;
    if (checkpoint == TlsServerProtocolCheckpoint::ClientHello) {
        try {
            // RFC 8446 Sections 4.1.3 and 4.2.8: create fresh independent server random and X25519 inputs only after
            // application policy accepts the fully parsed ClientHello checkpoint.
            auto random = core::application().secureRandom().buildByteBlock(unit::ByteLength{32U});
            auto privateKey = cryptology::KeyAgreementPrivateKey::generate(cryptology::KeyAgreementAlgorithm::X25519);
            resumeWithInputs(std::move(random), std::move(privateKey));
        } catch (const TlsProtocolError &error) {
            fail(error.alert(), error.reason());
        } catch (const err::Exception &error) {
            fail(TlsAlertDescription::InternalError, error.reason());
        } catch (...) {
            fail(TlsAlertDescription::InternalError, "TLS server flight generation failed unexpectedly."_el);
        }
        return;
    }
    processRetainedInputNoThrow();
}

void TlsServerProtocol::resumeWithInputs(mem::ByteBlock random, cryptology::KeyAgreementPrivateKey privateKey) {
    if (_state != TlsServerProtocolState::Handshaking || _handshakeStep != HandshakeStep::ClientHello ||
        _clientHello.isEmpty() || !_cipherSuite.has_value() || !_signatureScheme.has_value()) {
        throw err::LogicError{"TLS server ClientHello checkpoint is not ready for flight generation."_el};
    }
    buildServerFlight(std::move(random), std::move(privateKey));
    processRetainedInputNoThrow();
}

auto TlsServerProtocol::takeTransportOutput() -> std::optional<mem::ByteBlock> {
    if (_transportOutput.empty()) {
        return {};
    }
    auto result = std::move(_transportOutput.front());
    _transportOutput.pop_front();
    _transportOutputBytes -= result.length().toSizeT();
    if (_state == TlsServerProtocolState::Closing && _localCloseNotify && _peerCloseNotify &&
        _transportOutput.empty()) {
        // RFC 8446 Section 6.1: after both close_notify alerts and final transport handoff, no traffic secret can be
        // used again. Erase the terminal connection state at the exact last-record ownership transfer.
        eraseSecurityState();
        _state = TlsServerProtocolState::Closed;
    }
    return result;
}

auto TlsServerProtocol::takeApplicationData() -> std::optional<mem::ByteBlock> {
    if (_applicationInput.empty()) {
        return {};
    }
    auto result = std::move(_applicationInput.front());
    _applicationInput.pop_front();
    _applicationInputBytes -= result.length().toSizeT();
    return result;
}

auto TlsServerProtocol::queueTransport(mem::ByteBlock record) -> bool {
    const auto limit = _options.bufferLimits().send().toSizeT();
    const auto length = record.length().toSizeT();
    if (length > limit || _transportOutputBytes > limit - length) {
        return false;
    }
    _transportOutputBytes += length;
    _transportOutput.push_back(std::move(record));
    return true;
}

}
