// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientProtocol.hpp"

#include "TlsClientHelloBuilder.hpp"

#include "../../../../core/impl/application_data/ApplicationRandomData.hpp"
#include "../../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../random/Random.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace mem;
using namespace text::literals;

TlsClientProtocol::TlsClientProtocol(TlsClientProtocolOptions options) :
    _options{std::move(options)},
    _clientHelloBuilder{_options.host(), _options.alpnProtocols(), _options.supportedGroups()},
    _recordStream{_options.bufferLimits().receive()} {
    if (!_options.validationTime().isValid()) {
        throw ParameterFailure{"TLS certificate validation requires a valid explicit time."_el, "options"_el};
    }
}

TlsClientProtocol::~TlsClientProtocol() {
    eraseSecurityState();
}

void TlsClientProtocol::start() {
    if (_state != TlsClientProtocolState::Inactive) {
        throw LogicFailure{"The TLS client protocol has already been started."_el};
    }

    // RFC 8446 sections 4.1.2 and appendix D.4: generate independent 32-byte random and legacy session ID values.
    const auto randomData = core::impl::applicationData()->random().get();
    auto random = randomData->secureRandom().buildByteBlock(unit::ByteCount{32U});
    auto legacySessionId = randomData->secureRandom().buildByteBlock(unit::ByteCount{32U});

    // RFC 8446 sections 4.2.7 and 4.2.8: generate the only offered X25519 key share before ClientHello.
    auto privateKey = cryptology::KeyAgreementPrivateKey::generate(_options.supportedGroups().front());
    startWithInputs(std::move(random), std::move(legacySessionId), std::move(privateKey));
}

void TlsClientProtocol::startWithInputs(
    ByteBlock random, ByteBlock legacySessionId, cryptology::KeyAgreementPrivateKey privateKey) {
    if (_state != TlsClientProtocolState::Inactive) {
        throw LogicFailure{"The TLS client protocol has already been started."_el};
    }

    const auto publicKey = privateKey.publicKey();

    // RFC 8446 section 4.1.2: construct the complete ClientHello once; the exact bytes are retained for the transcript.
    auto clientHello = _clientHelloBuilder.buildHandshake(random.span(), legacySessionId.span(), publicKey);
    auto initialRecord = _clientHelloBuilder.buildInitialRecord(clientHello.span());
    if (initialRecord.length().toSizeT() > _options.bufferLimits().send().toSizeT()) {
        throw ParameterFailure{"The TLS send limit cannot hold the initial ClientHello record."_el, "options"_el};
    }

    // Transfer the ephemeral private key only after every local serialization and capacity check has succeeded.
    _privateKey = std::move(privateKey);
    _clientHello = std::move(clientHello);
    _legacySessionId = std::move(legacySessionId);
    if (!queueTransport(std::move(initialRecord))) {
        throw LogicFailure{"The validated TLS ClientHello did not fit the empty output queue."_el};
    }
    _state = TlsClientProtocolState::Handshaking;
    _handshakeStep = HandshakeStep::ServerHello;
}

auto TlsClientProtocol::takeTransportOutput() -> std::optional<ByteBlock> {
    if (_transportOutput.empty()) {
        return {};
    }
    auto result = std::move(_transportOutput.front());
    _transportOutput.pop_front();
    _transportOutputBytes -= result.length().toSizeT();
    if (_state == TlsClientProtocolState::Closing && _localCloseNotify && _peerCloseNotify &&
        _transportOutput.empty()) {
        _state = TlsClientProtocolState::Closed;
    }
    return result;
}

auto TlsClientProtocol::takeApplicationData() -> std::optional<ByteBlock> {
    if (_applicationInput.empty()) {
        return {};
    }
    auto result = std::move(_applicationInput.front());
    _applicationInput.pop_front();
    _applicationInputBytes -= result.length().toSizeT();
    return result;
}

auto TlsClientProtocol::queueTransport(ByteBlock record) -> bool {
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
