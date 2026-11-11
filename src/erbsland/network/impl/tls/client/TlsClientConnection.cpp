// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientConnection.hpp"

#include "TlsClientProtocolOptions.hpp"

#include "../../../../core/impl/application_data/ApplicationCryptologyData.hpp"
#include "../../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../event/EventTarget.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../time/Timestamp.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace text;
using namespace text::literals;

TlsClientConnection::TlsClientConnection(
    event::EventTargetPtr ownerLoop, network::TcpConnectionPtr tcpConnection, TlsClientProtocolStartFn startProtocol) :
    network::TlsClientConnection{std::move(ownerLoop)},
    _tcpConnection{std::move(tcpConnection)},
    _startProtocol{std::move(startProtocol)} {
    if (_tcpConnection == nullptr || _tcpConnection->ownerLoop() != this->ownerLoop()) {
        throw LogicFailure{"A TLS client connection requires a TCP connection from the same owner loop."_el};
    }
    if (!_startProtocol) {
        _startProtocol = [](TlsClientProtocol &protocol) -> void { protocol.start(); };
    }
    _finalContinuation = event::impl::EventContinuation::create(this->ownerLoop());
}

TlsClientConnection::~TlsClientConnection() {
    cancelHandshakeTimer();
}

auto TlsClientConnection::requestedConfigurationLabel() const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return _requestedConfigurationLabel;
}

auto TlsClientConnection::matchedConfigurationLabel() const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return _matchedConfigurationLabel;
}

auto TlsClientConnection::requestedEndpoint() const -> std::optional<HostEndpoint> {
    const auto lock = std::scoped_lock{_mutex};
    return _requestedEndpoint;
}

auto TlsClientConnection::localEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->localEndpoint();
}

auto TlsClientConnection::remoteEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->remoteEndpoint();
}

auto TlsClientConnection::cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::nullopt : _protocol->cipherSuite();
}

auto TlsClientConnection::negotiatedAlpn() const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? String{} : _protocol->negotiatedAlpn();
}

auto TlsClientConnection::peerCertificatePath() const -> util::List<cryptology::X509Certificate> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? util::List<cryptology::X509Certificate>{} : _protocol->validatedPath();
}

auto TlsClientConnection::bufferLimits() const noexcept -> SocketBufferLimits {
    const auto lock = std::scoped_lock{_mutex};
    return _options.bufferLimits();
}

auto TlsClientConnection::state() const noexcept -> ConnectionState {
    return _state.load();
}

void TlsClientConnection::connect(HostEndpoint endpoint, TlsClientConnectOptions options) {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_started.load()) {
        throw LogicFailure{"A TLS client connection can only be started once."_el};
    }
    validateOptions(endpoint, options);

    // Resolve and validate the complete selected entry before consuming either one-shot source.
    const auto cryptologyData = core::impl::applicationData()->cryptology().get();
    const auto resolution = cryptologyData->configuration().resolveTlsConfiguration(
        options.configurationLabel(), cryptology::TlsRole::Client);
    auto protocol = std::make_unique<TlsClientProtocol>(TlsClientProtocolOptions{
        endpoint.host(),
        *resolution.certificatePolicy(),
        time::Timestamp::now(),
        options.alpnProtocols(),
        options.bufferLimits(),
        options.supportedGroups(),
        resolution.configuration()->client()->identity()});

    configureTcpEvents();
    _requestedEndpoint = endpoint;
    _requestedConfigurationLabel = resolution.requestedLabel();
    _matchedConfigurationLabel = resolution.matchedLabel();
    _configuration = resolution.configuration();
    _options = std::move(options);
    _protocol = std::move(protocol);
    _started.store(true);
    _state.store(ConnectionState::Connecting);
    _tcpConnection->connect(std::move(endpoint), _options.tcpOptions());
}

auto TlsClientConnection::send(const mem::ByteBlock &data) -> NetworkSendStatus {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != ConnectionState::Active || _protocol == nullptr) {
        return NetworkSendStatus::Closed;
    }
    if (data.length() > TlsClientConnectOptions::cMaximumApplicationSendLength) {
        throw ParameterFailure{"One TLS application send cannot exceed 16 KiB."_el, "data"_el};
    }
    const auto status = _protocol->sendApplication(data.span());
    if (status == NetworkSendStatus::WouldBlock) {
        _applicationBlocked = true;
        return status;
    }
    if (status == NetworkSendStatus::Accepted) {
        pumpTransportOutput();
        if (!data.isEmpty()) {
            recordApplicationActivity();
        }
    }
    return status;
}

void TlsClientConnection::pauseReceiving() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != ConnectionState::Active || _receivingPaused) {
        return;
    }
    _receivingPaused = true;
    _tcpConnection->pauseReceiving();
}

void TlsClientConnection::resumeReceiving() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != ConnectionState::Active || !_receivingPaused) {
        return;
    }
    _receivingPaused = false;
    drainApplicationInput();
    if (canContinue()) {
        _tcpConnection->resumeReceiving();
    }
}

void TlsClientConnection::close() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() == ConnectionState::Inactive || _state.load() == ConnectionState::Connecting ||
        _state.load() == ConnectionState::Handshaking) {
        abort();
        return;
    }
    if (_state.load() != ConnectionState::Active || _protocol == nullptr) {
        return;
    }
    enterClosing(ConnectionCloseOrigin::Local);
    _protocol->close();
    pumpTransportOutput();
    assessProtocolState();
}

void TlsClientConnection::abort() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    const auto previous = _state.exchange(ConnectionState::Closed);
    if (previous == ConnectionState::Closed || previous == ConnectionState::Failed) {
        return;
    }
    _started.store(true);
    _abortRequested.store(true);
    _backgroundOperation.cancel();
    cancelHandshakeTimer();
    ++_backgroundGeneration;
    ++_handshakeTimerGeneration;
    ++_idleTimerGeneration;
    ++_closeTimerGeneration;
    _tcpConnection->abort();
    if (previous == ConnectionState::Inactive) {
        postAbortFinal();
    }
}

auto TlsClientConnection::events() -> network::TlsClientConnectionEventEditor & {
    const auto lock = std::scoped_lock{_mutex};
    auto target = currentOwnerLoop();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<TlsClientConnectionEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

void TlsClientConnection::validateOptions(const HostEndpoint &endpoint, const TlsClientConnectOptions &options) {
    const auto tlsLimits = options.bufferLimits();
    if (!tlsLimits.send().isFinite() || tlsLimits.send().isZero() ||
        tlsLimits.send() > TlsClientConnectOptions::cMaximumBufferLength) {
        throw ParameterFailure{
            "The TLS send queue limit must be positive, finite, and at most 16 MiB."_el, "options.bufferLimits"_el};
    }
    if (!tlsLimits.receive().isFinite() || tlsLimits.receive().isZero() ||
        tlsLimits.receive() > TlsClientConnectOptions::cMaximumBufferLength) {
        throw ParameterFailure{
            "The TLS receive limit must be positive, finite, and at most 16 MiB."_el, "options.bufferLimits"_el};
    }
    if (!options.handshakeTimeout().isPositive() || !options.idleTimeout().isPositive() ||
        !options.closeTimeout().isPositive()) {
        throw ParameterFailure{"Every TLS deadline must be positive."_el, "options"_el};
    }
    if (!options.tcpOptions().timeout().isPositive()) {
        throw ParameterFailure{"The TCP connection timeout must be positive."_el, "options.tcpOptions"_el};
    }
    const auto tcpLimits = options.tcpOptions().bufferLimits();
    if (!tcpLimits.send().isFinite() || tcpLimits.send().isZero() || !tcpLimits.receive().isFinite() ||
        tcpLimits.receive().isZero()) {
        throw ParameterFailure{"The TCP queue limits must be positive and finite."_el, "options.tcpOptions"_el};
    }
    if (endpoint.port().isAutomatic()) {
        throw ParameterFailure{"A TLS destination port must not be automatic."_el, "endpoint"_el};
    }
    if (endpoint.host().isAddress() && endpoint.host().address()->isV4() && endpoint.scopeId().isSpecified()) {
        throw ParameterFailure{"An IPv4 TLS destination cannot contain an IPv6 scope."_el, "endpoint"_el};
    }
}

auto TlsClientConnection::canContinue() const noexcept -> bool {
    const auto current = _state.load();
    return !_abortRequested.load() && current != ConnectionState::Closed && current != ConnectionState::Failed;
}

}
