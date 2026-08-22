// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientConnection.hpp"

#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsClientConnection::recordApplicationActivity() {
    _idleDeadline = time::TimePoint::now() + _options.idleTimeout();
    ensureIdleTimer();
}

void TlsClientConnection::ensureIdleTimer() {
    if (_idleTimerPending || _state.load() != ConnectionState::Active) {
        return;
    }
    _idleTimerPending = true;
    const auto generation = _idleTimerGeneration;
    const auto delay = time::TimePoint::now().timeDeltaTo(_idleDeadline);
    const auto weakSelf =
        std::weak_ptr<TlsClientConnection>{std::static_pointer_cast<TlsClientConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(delay, [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleIdleTimeout(generation);
        }
    });
}

void TlsClientConnection::handleHandshakeTimeout(const std::uint64_t generation, const time::TimePoint deadline) {
    const auto lock = std::scoped_lock{_mutex};
    if (generation != _handshakeTimerGeneration || _state.load() != ConnectionState::Handshaking ||
        time::TimePoint::now() < deadline) {
        return;
    }
    if (_protocol != nullptr) {
        _protocol->timeout();
    }
    finishFailed(
        NetworkErrorContext{"TLS handshake timed out"_el, "The TLS handshake deadline expired."_el}
            .setReason(NetworkErrorReason::Timeout)
            .setPhase(NetworkErrorPhase::Handshaking));
}

void TlsClientConnection::handleIdleTimeout(const std::uint64_t generation) {
    const auto lock = std::scoped_lock{_mutex};
    if (generation != _idleTimerGeneration) {
        return;
    }
    _idleTimerPending = false;
    if (_state.load() != ConnectionState::Active) {
        return;
    }
    if (time::TimePoint::now() < _idleDeadline) {
        ensureIdleTimer();
        return;
    }
    finishFailed(
        NetworkErrorContext{"TLS connection timed out"_el, "The authenticated application idle deadline expired."_el}
            .setReason(NetworkErrorReason::Timeout)
            .setPhase(NetworkErrorPhase::Active));
}

void TlsClientConnection::handleCloseTimeout(const std::uint64_t generation, const time::TimePoint deadline) {
    const auto lock = std::scoped_lock{_mutex};
    if (generation != _closeTimerGeneration || _state.load() != ConnectionState::Closing ||
        time::TimePoint::now() < deadline) {
        return;
    }
    finishFailed(
        NetworkErrorContext{"TLS closure timed out"_el, "The peer did not complete bidirectional TLS closure."_el}
            .setReason(NetworkErrorReason::Timeout)
            .setPhase(NetworkErrorPhase::Closing));
}

void TlsClientConnection::enterClosing(const ConnectionCloseOrigin origin) {
    if (_closeOrigin.has_value()) {
        return;
    }
    _closeOrigin = origin;
    _state.store(ConnectionState::Closing);
    ++_idleTimerGeneration;
    _idleTimerPending = false;
    const auto generation = ++_closeTimerGeneration;
    const auto deadline = time::TimePoint::now() + _options.closeTimeout();
    const auto weakSelf =
        std::weak_ptr<TlsClientConnection>{std::static_pointer_cast<TlsClientConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.closeTimeout(), [weakSelf, generation, deadline]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleCloseTimeout(generation, deadline);
        }
    });
}

auto TlsClientConnection::protocolErrorContext() const -> NetworkErrorContext {
    auto context = NetworkErrorContext{
        "TLS connection failed"_el,
        _protocol == nullptr || _protocol->failureDiagnostic().isEmpty() ? "The TLS protocol failed."_el
                                                                         : _protocol->failureDiagnostic()};
    const auto phase = _state.load() == ConnectionState::Closing ? NetworkErrorPhase::Closing
        : _state.load() == ConnectionState::Active               ? NetworkErrorPhase::Active
                                                                 : NetworkErrorPhase::Handshaking;
    context.setPhase(phase);
    if (_protocol == nullptr) {
        return context.setReason(NetworkErrorReason::TlsInternalFailure);
    }
    if (_protocol->failureAlert().has_value()) {
        context.setTlsAlert(*_protocol->failureAlert());
    }
    if (_protocol->failureWasTruncation()) {
        return context.setReason(NetworkErrorReason::TlsTruncation);
    }
    if (_protocol->failureWasPeerAlert()) {
        return context.setReason(NetworkErrorReason::TlsPeerAlert);
    }
    if (!_protocol->failureAlert().has_value()) {
        return context.setReason(NetworkErrorReason::TlsInternalFailure);
    }
    switch (*_protocol->failureAlert()) {
    case TlsAlertDescription::BadCertificate:
    case TlsAlertDescription::UnsupportedCertificate:
    case TlsAlertDescription::CertificateRevoked:
    case TlsAlertDescription::CertificateExpired:
    case TlsAlertDescription::CertificateUnknown:
    case TlsAlertDescription::UnknownCa:
    case TlsAlertDescription::DecryptError:
    case TlsAlertDescription::CertificateRequired:
        return context.setReason(NetworkErrorReason::TlsAuthenticationFailure);
    case TlsAlertDescription::AccessDenied:
    case TlsAlertDescription::InsufficientSecurity:
    case TlsAlertDescription::NoApplicationProtocol:
        return context.setReason(NetworkErrorReason::TlsPolicyFailure);
    case TlsAlertDescription::InternalError:
        return context.setReason(NetworkErrorReason::TlsInternalFailure);
    default:
        return context.setReason(NetworkErrorReason::TlsProtocolFailure);
    }
}

auto TlsClientConnection::enrichTransportError(NetworkErrorContext context) const -> NetworkErrorContext {
    auto phase = NetworkErrorPhase::Connecting;
    switch (_state.load()) {
    case ConnectionState::Inactive:
        phase = NetworkErrorPhase::Configuration;
        break;
    case ConnectionState::Connecting:
        phase = _hostResolved ? NetworkErrorPhase::Connecting : NetworkErrorPhase::Resolving;
        break;
    case ConnectionState::Accepting:
    case ConnectionState::Handshaking:
        phase = NetworkErrorPhase::Handshaking;
        break;
    case ConnectionState::Active:
        phase = NetworkErrorPhase::Active;
        break;
    case ConnectionState::Closing:
    case ConnectionState::Closed:
        phase = NetworkErrorPhase::Closing;
        break;
    case ConnectionState::Failed:
        phase = context.phase();
        break;
    }
    context.setPhase(phase);
    if (!context.localEndpoint().has_value()) {
        if (const auto endpoint = _tcpConnection->localEndpoint(); endpoint.has_value()) {
            context.setLocalEndpoint(*endpoint);
        }
    }
    if (!context.remoteEndpoint().has_value() && _requestedEndpoint.has_value()) {
        context.setRemoteEndpoint(*_requestedEndpoint);
    }
    return context;
}

void TlsClientConnection::finishFailed(NetworkErrorContext context) {
    if (_finalized.load() || _state.load() == ConnectionState::Failed) {
        return;
    }
    _state.store(ConnectionState::Failed);
    ++_handshakeTimerGeneration;
    ++_idleTimerGeneration;
    ++_closeTimerGeneration;
    _idleTimerPending = false;
    if (_protocol != nullptr) {
        _protocol->abort();
    }
    try {
        _tcpConnection->abort();
    } catch (...) {}
    try {
        const auto callback = _onError;
        if (callback) {
            callback(context);
        }
    } catch (...) {
        finishFinal();
        throw;
    }
    finishFinal();
}

void TlsClientConnection::finishClosed() {
    if (_finalized.load()) {
        return;
    }
    _state.store(ConnectionState::Closed);
    ++_closeTimerGeneration;
    const auto context = ConnectionCloseContext{_closeOrigin.value_or(ConnectionCloseOrigin::Remote)};
    try {
        const auto callback = _onClosed;
        if (callback) {
            callback(context);
        }
    } catch (...) {
        finishFinal();
        throw;
    }
    finishFinal();
}

void TlsClientConnection::finishFinal() {
    if (_finalized.exchange(true)) {
        return;
    }
    const auto callback = _onFinal;
    if (callback) {
        callback();
    }
}

void TlsClientConnection::postAbortFinal() {
    const auto weakSelf =
        std::weak_ptr<TlsClientConnection>{std::static_pointer_cast<TlsClientConnection>(shared_from_this())};
    try {
        ownerEvents()->invoke([weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                const auto lock = std::scoped_lock{self->_mutex};
                self->finishFinal();
            }
        });
    } catch (...) {}
}

}
