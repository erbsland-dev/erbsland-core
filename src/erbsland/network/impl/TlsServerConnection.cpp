// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerConnection.hpp"

#include "TlsServerProtocolOptions.hpp"

#include "../tcp/TcpConnectionRequest.hpp"

#include "../../core/Application.hpp"
#include "../../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

TlsServerConnection::TlsServerConnection(
    event::EventsPtr ownerEvents, network::TcpConnectionPtr tcpConnection, StartProtocolFn startProtocol) :
    network::TlsServerConnection{std::move(ownerEvents)},
    _tcpConnection{std::move(tcpConnection)},
    _startProtocol{std::move(startProtocol)} {
    if (_tcpConnection == nullptr || _tcpConnection->ownerEvents() != this->ownerEvents()) {
        throw err::LogicError{"A TLS server connection requires a TCP connection from the same owner loop."_el};
    }
    if (!_startProtocol) {
        _startProtocol = [](TlsServerProtocol &protocol) -> void { protocol.start(); };
    }
}

TlsServerConnection::~TlsServerConnection() = default;

auto TlsServerConnection::requestedConfigurationLabel() const -> std::optional<text::String> {
    const auto lock = std::scoped_lock{_mutex};
    if (_protocol == nullptr || !_protocol->selectedIdentityIndex().has_value()) {
        return std::nullopt;
    }
    return _identities[*_protocol->selectedIdentityIndex()].requestedLabel;
}

auto TlsServerConnection::matchedConfigurationLabel() const -> std::optional<text::String> {
    const auto lock = std::scoped_lock{_mutex};
    if (_protocol == nullptr || !_protocol->selectedIdentityIndex().has_value()) {
        return std::nullopt;
    }
    return _identities[*_protocol->selectedIdentityIndex()].matchedLabel;
}

auto TlsServerConnection::localEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->localEndpoint();
}

auto TlsServerConnection::remoteEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->remoteEndpoint();
}

auto TlsServerConnection::serverName() const -> std::optional<HostName> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::nullopt : _protocol->serverName();
}

auto TlsServerConnection::offeredAlpn() const -> std::vector<mem::ByteBlock> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::vector<mem::ByteBlock>{} : _protocol->offeredAlpn();
}

auto TlsServerConnection::negotiatedAlpn() const -> mem::ByteBlock {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? mem::ByteBlock{} : _protocol->negotiatedAlpn();
}

auto TlsServerConnection::cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::nullopt : _protocol->cipherSuite();
}

auto TlsServerConnection::signatureScheme() const -> std::optional<cryptology::TlsSignatureScheme> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::nullopt : _protocol->signatureScheme();
}

auto TlsServerConnection::bufferLimits() const noexcept -> SocketBufferLimits {
    const auto lock = std::scoped_lock{_mutex};
    return _options.bufferLimits();
}

auto TlsServerConnection::state() const noexcept -> TlsServerConnectionState {
    return _state.load();
}

void TlsServerConnection::accept(network::TcpConnectionRequestPtr request, TlsServerAcceptOptions options) {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerEvents();
    if (_started.load()) {
        throw err::LogicError{"A TLS server connection can only be started once."_el};
    }
    validateOptions(options);
    if (request == nullptr) {
        throw err::ParameterError{"The TCP connection request must not be empty."_el, "request"_el};
    }

    auto identities = resolveIdentities(options);
    auto namedIdentities = std::vector<TlsServerProtocolOptions::NamedIdentity>{};
    namedIdentities.reserve(options.identityMappings().size());
    for (auto index = std::size_t{}; index < options.identityMappings().size(); ++index) {
        namedIdentities.push_back(
            TlsServerProtocolOptions::NamedIdentity{
                options.identityMappings()[index].serverName(),
                identities[index + 1U].configuration->serverIdentity()});
    }
    auto protocol = std::make_unique<TlsServerProtocol>(TlsServerProtocolOptions::withNamedIdentities(
        identities.front().configuration->serverIdentity(),
        std::move(namedIdentities),
        options.alpnProtocols(),
        options.cipherSuites(),
        options.bufferLimits()));

    auto handshakeLease = options.handshakeQuota()->tryAcquire(request->remoteEndpoint());
    if (!handshakeLease.has_value()) {
        _started.store(true);
        _state.store(TlsServerConnectionState::Failed);
        const auto remote = request->remoteEndpoint();
        request->reject();
        postAdmissionFailure(
            NetworkErrorContext{"TLS connection rejected"_el, "The shared TLS handshake quota is full."_el}
                .setReason(NetworkErrorReason::ResourceLimitExceeded)
                .setPhase(NetworkErrorPhase::Accepting)
                .setRemoteEndpoint(HostEndpoint{remote.address(), remote.port(), remote.scopeId()}));
        return;
    }

    configureTcpEvents();
    _identities = std::move(identities);
    _options = std::move(options);
    _protocol = std::move(protocol);
    _handshakeLease = std::move(*handshakeLease);
    _started.store(true);
    _state.store(TlsServerConnectionState::Accepting);
    try {
        _tcpConnection->accept(std::move(request), _options.tcpOptions());
    } catch (...) {
        _state.store(TlsServerConnectionState::Inactive);
        _started.store(false);
        _handshakeLease.release();
        _protocol.reset();
        _identities.clear();
        throw;
    }
}

auto TlsServerConnection::send(const mem::ByteBlock &data) -> NetworkSendStatus {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != TlsServerConnectionState::Active || _protocol == nullptr) {
        return NetworkSendStatus::Closed;
    }
    if (data.length() > TlsServerAcceptOptions::cMaximumApplicationSendLength) {
        throw err::ParameterError{"One TLS application send cannot exceed 16 KiB."_el, "data"_el};
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

void TlsServerConnection::pauseReceiving() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != TlsServerConnectionState::Active || _receivingPaused) {
        return;
    }
    _receivingPaused = true;
    _tcpConnection->pauseReceiving();
}

void TlsServerConnection::resumeReceiving() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != TlsServerConnectionState::Active || !_receivingPaused) {
        return;
    }
    _receivingPaused = false;
    drainApplicationInput();
    if (canContinue()) {
        _tcpConnection->resumeReceiving();
    }
}

void TlsServerConnection::close() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerEvents();
    const auto current = _state.load();
    if (current == TlsServerConnectionState::Inactive || current == TlsServerConnectionState::Accepting ||
        current == TlsServerConnectionState::Handshaking) {
        abort();
        return;
    }
    if (current != TlsServerConnectionState::Active || _protocol == nullptr) {
        return;
    }
    enterClosing(TlsServerConnectionCloseOrigin::Local);
    _protocol->close();
    pumpTransportOutput();
    assessProtocolState();
}

void TlsServerConnection::abort() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    const auto previous = _state.exchange(TlsServerConnectionState::Closed);
    if (previous == TlsServerConnectionState::Closed || previous == TlsServerConnectionState::Failed) {
        return;
    }
    _started.store(true);
    _abortRequested.store(true);
    ++_handshakeTimerGeneration;
    ++_idleTimerGeneration;
    ++_closeTimerGeneration;
    _handshakeLease.release();
    try {
        _tcpConnection->abort();
    } catch (...) {}
    if (previous == TlsServerConnectionState::Inactive) {
        postAbortFinal();
    }
}

auto TlsServerConnection::events() -> network::TlsServerConnectionEventEditor & {
    const auto lock = std::scoped_lock{_mutex};
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<TlsServerConnectionEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

void TlsServerConnection::validateOptions(const TlsServerAcceptOptions &options) {
    if (options.handshakeQuota() == nullptr) {
        throw err::ParameterError{"The TLS handshake quota must not be empty."_el, "options"_el};
    }
    if (options.identityMappings().size() > TlsServerAcceptOptions::cMaximumIdentityMappings.toSizeT()) {
        throw err::ParameterError{"TLS server options exceed 64 exact identity mappings."_el, "options"_el};
    }
    const auto limits = options.bufferLimits();
    if (!limits.send().isFinite() || limits.send().isZero() ||
        limits.send() > TlsServerAcceptOptions::cMaximumBufferLength || !limits.receive().isFinite() ||
        limits.receive().isZero() || limits.receive() > TlsServerAcceptOptions::cMaximumBufferLength) {
        throw err::ParameterError{"TLS queue limits must be positive, finite, and at most 16 MiB."_el, "options"_el};
    }
    const auto tcpLimits = options.tcpOptions().bufferLimits();
    if (!tcpLimits.send().isFinite() || tcpLimits.send().isZero() || !tcpLimits.receive().isFinite() ||
        tcpLimits.receive().isZero()) {
        throw err::ParameterError{"TCP queue limits must be positive and finite."_el, "options"_el};
    }
    if (!options.handshakeTimeout().isPositive() || !options.idleTimeout().isPositive() ||
        !options.closeTimeout().isPositive()) {
        throw err::ParameterError{"Every TLS deadline must be positive."_el, "options"_el};
    }
}

auto TlsServerConnection::resolveIdentities(const TlsServerAcceptOptions &options) -> std::vector<ResolvedIdentity> {
    auto result = std::vector<ResolvedIdentity>{};
    result.reserve(options.identityMappings().size() + 1U);
    const auto resolve = [&result](const text::String &label) -> void {
        const auto resolution = core::application().cryptologyConfiguration().resolveTlsConfiguration(label);
        if (!resolution.configuration()->hasServerIdentity()) {
            throw err::RuntimeError{text::String::fromJoined(
                {"TLS configuration \""_el, resolution.matchedLabel(), "\" has no server identity."_el})};
        }
        result.push_back(
            ResolvedIdentity{resolution.requestedLabel(), resolution.matchedLabel(), resolution.configuration()});
    };
    resolve(options.configurationLabel());
    for (const auto &mapping : options.identityMappings()) {
        resolve(mapping.configurationLabel());
    }
    return result;
}

auto TlsServerConnection::canContinue() const noexcept -> bool {
    const auto current = _state.load();
    return !_abortRequested.load() && current != TlsServerConnectionState::Closed &&
        current != TlsServerConnectionState::Failed;
}

}
