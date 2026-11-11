// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerConnection.hpp"

#include "TlsServerProtocolOptions.hpp"

#include "../../../../core/impl/application_data/ApplicationCryptologyData.hpp"
#include "../../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../event/EventTarget.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../tcp/TcpConnectionRequest.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace text;
using namespace text::literals;

TlsServerConnection::TlsServerConnection(
    event::EventTargetPtr ownerLoop, network::TcpConnectionPtr tcpConnection, StartProtocolFn startProtocol) :
    network::TlsServerConnection{std::move(ownerLoop)},
    _tcpConnection{std::move(tcpConnection)},
    _startProtocol{std::move(startProtocol)} {
    if (_tcpConnection == nullptr || _tcpConnection->ownerLoop() != this->ownerLoop()) {
        throw LogicFailure{"A TLS server connection requires a TCP connection from the same owner loop."_el};
    }
    if (!_startProtocol) {
        _startProtocol = [](TlsServerProtocol &protocol) -> void { protocol.start(); };
    }
    _finalContinuation = event::impl::EventContinuation::create(this->ownerLoop());
}

TlsServerConnection::~TlsServerConnection() {
    cancelHandshakeTimer();
}

auto TlsServerConnection::requestedConfigurationLabel() const -> std::optional<String> {
    const auto lock = std::scoped_lock{_mutex};
    if (_protocol == nullptr || !_protocol->selectedConfigurationIndex().has_value()) {
        return std::nullopt;
    }
    return _configurations[*_protocol->selectedConfigurationIndex()].requestedLabel;
}

auto TlsServerConnection::matchedConfigurationLabel() const -> std::optional<String> {
    const auto lock = std::scoped_lock{_mutex};
    if (_protocol == nullptr || !_protocol->selectedConfigurationIndex().has_value()) {
        return std::nullopt;
    }
    return _configurations[*_protocol->selectedConfigurationIndex()].matchedLabel;
}

auto TlsServerConnection::localEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->localEndpoint();
}

auto TlsServerConnection::remoteEndpoint() const -> std::optional<IpEndpoint> {
    return _tcpConnection->remoteEndpoint();
}

auto TlsServerConnection::peerCertificatePath() const -> util::List<cryptology::X509Certificate> {
    const auto lock = std::scoped_lock{_mutex};
    if (_protocol == nullptr) {
        return {};
    }
    return _protocol->peerCertificatePath();
}

auto TlsServerConnection::isPeerAuthenticated() const -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol != nullptr && _protocol->isPeerAuthenticated();
}

auto TlsServerConnection::serverName() const -> std::optional<HostName> {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? std::nullopt : _protocol->serverName();
}

auto TlsServerConnection::offeredAlpn() const -> StringList {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? StringList{} : _protocol->offeredAlpn();
}

auto TlsServerConnection::negotiatedAlpn() const -> String {
    const auto lock = std::scoped_lock{_mutex};
    return _protocol == nullptr ? String{} : _protocol->negotiatedAlpn();
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

auto TlsServerConnection::state() const noexcept -> ConnectionState {
    return _state.load();
}

void TlsServerConnection::accept(network::TcpConnectionRequestPtr request, TlsServerAcceptOptions options) {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_started.load()) {
        throw LogicFailure{"A TLS server connection can only be started once."_el};
    }
    validateOptions(options);
    if (request == nullptr) {
        throw ParameterFailure{"The TCP connection request must not be empty."_el, "request"_el};
    }

    auto configurations = resolveConfigurations(options);
    auto namedConfigurations = std::vector<TlsServerProtocolOptions::NamedConfiguration>{};
    namedConfigurations.reserve(options.configurationMappings().size());
    for (auto index = std::size_t{}; index < options.configurationMappings().size(); ++index) {
        namedConfigurations.push_back(
            TlsServerProtocolOptions::NamedConfiguration{
                options.configurationMappings()[index].serverName(),
                TlsServerProtocolConfiguration{
                    configurations[index + 1U].configuration->server()->identity(),
                    configurations[index + 1U].configuration->server()->clientAuthentication(),
                    configurations[index + 1U].certificatePolicy}});
    }
    auto protocol = std::make_unique<TlsServerProtocol>(TlsServerProtocolOptions::withNamedConfigurations(
        TlsServerProtocolConfiguration{
            configurations.front().configuration->server()->identity(),
            configurations.front().configuration->server()->clientAuthentication(),
            configurations.front().certificatePolicy},
        std::move(namedConfigurations),
        options.alpnProtocols(),
        options.cipherSuites(),
        options.bufferLimits(),
        options.supportedGroups()));

    auto handshakeLease = options.handshakeQuota()->tryAcquire(request->remoteEndpoint());
    if (!handshakeLease.has_value()) {
        _started.store(true);
        _state.store(ConnectionState::Failed);
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
    _configurations = std::move(configurations);
    _options = std::move(options);
    _protocol = std::move(protocol);
    _handshakeLease = std::move(*handshakeLease);
    _started.store(true);
    _state.store(ConnectionState::Accepting);
    try {
        _tcpConnection->accept(std::move(request), _options.tcpOptions());
    } catch (...) {
        _state.store(ConnectionState::Inactive);
        _started.store(false);
        _handshakeLease.release();
        _protocol.reset();
        _configurations.clear();
        throw;
    }
}

auto TlsServerConnection::send(const mem::ByteBlock &data) -> NetworkSendStatus {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != ConnectionState::Active || _protocol == nullptr) {
        return NetworkSendStatus::Closed;
    }
    if (data.length() > TlsServerAcceptOptions::cMaximumApplicationSendLength) {
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

void TlsServerConnection::pauseReceiving() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != ConnectionState::Active || _receivingPaused) {
        return;
    }
    _receivingPaused = true;
    _tcpConnection->pauseReceiving();
}

void TlsServerConnection::resumeReceiving() {
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

void TlsServerConnection::close() {
    const auto lock = std::scoped_lock{_mutex};
    verifyCurrentOwnerLoop();
    const auto current = _state.load();
    if (current == ConnectionState::Inactive || current == ConnectionState::Accepting ||
        current == ConnectionState::Handshaking) {
        abort();
        return;
    }
    if (current != ConnectionState::Active || _protocol == nullptr) {
        return;
    }
    enterClosing(ConnectionCloseOrigin::Local);
    _protocol->close();
    pumpTransportOutput();
    assessProtocolState();
}

void TlsServerConnection::abort() noexcept {
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
    _handshakeLease.release();
    _tcpConnection->abort();
    if (previous == ConnectionState::Inactive) {
        postAbortFinal();
    }
}

auto TlsServerConnection::events() -> network::TlsServerConnectionEventEditor & {
    const auto lock = std::scoped_lock{_mutex};
    auto target = currentOwnerLoop();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<TlsServerConnectionEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

void TlsServerConnection::validateOptions(const TlsServerAcceptOptions &options) {
    if (options.handshakeQuota() == nullptr) {
        throw ParameterFailure{"The TLS handshake quota must not be empty."_el, "options"_el};
    }
    if (options.configurationMappings().size() > TlsServerAcceptOptions::cMaximumConfigurationMappings.toSizeT()) {
        throw ParameterFailure{"TLS server options exceed 64 exact configuration mappings."_el, "options"_el};
    }
    const auto limits = options.bufferLimits();
    if (!limits.send().isFinite() || limits.send().isZero() ||
        limits.send() > TlsServerAcceptOptions::cMaximumBufferLength || !limits.receive().isFinite() ||
        limits.receive().isZero() || limits.receive() > TlsServerAcceptOptions::cMaximumBufferLength) {
        throw ParameterFailure{"TLS queue limits must be positive, finite, and at most 16 MiB."_el, "options"_el};
    }
    const auto tcpLimits = options.tcpOptions().bufferLimits();
    if (!tcpLimits.send().isFinite() || tcpLimits.send().isZero() || !tcpLimits.receive().isFinite() ||
        tcpLimits.receive().isZero()) {
        throw ParameterFailure{"TCP queue limits must be positive and finite."_el, "options"_el};
    }
    if (!options.handshakeTimeout().isPositive() || !options.idleTimeout().isPositive() ||
        !options.closeTimeout().isPositive()) {
        throw ParameterFailure{"Every TLS deadline must be positive."_el, "options"_el};
    }
}

auto TlsServerConnection::resolveConfigurations(const TlsServerAcceptOptions &options)
    -> std::vector<ResolvedConfiguration> {
    auto result = std::vector<ResolvedConfiguration>{};
    result.reserve(options.configurationMappings().size() + 1U);
    auto labels = StringList{options.configurationLabel()};
    for (const auto &mapping : options.configurationMappings()) {
        labels.append(mapping.configurationLabel());
    }
    const auto cryptologyData = core::impl::applicationData()->cryptology().get();
    for (
        const auto &resolution :
        cryptologyData->configuration().resolveTlsConfigurations(labels, cryptology::TlsRole::Server)) {
        result.push_back(
            ResolvedConfiguration{
                resolution.requestedLabel(),
                resolution.matchedLabel(),
                resolution.configuration(),
                resolution.certificatePolicy()});
    }
    return result;
}

auto TlsServerConnection::canContinue() const noexcept -> bool {
    const auto current = _state.load();
    return !_abortRequested.load() && current != ConnectionState::Closed && current != ConnectionState::Failed;
}

}
