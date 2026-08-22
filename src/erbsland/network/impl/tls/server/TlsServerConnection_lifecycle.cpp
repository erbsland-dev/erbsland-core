// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerConnection.hpp"

#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"

#include <exception>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerConnection::recordApplicationActivity() {
    _idleDeadline = time::TimePoint::now() + _options.idleTimeout();
    ensureIdleTimer();
}

void TlsServerConnection::ensureIdleTimer() {
    if (_idleTimerPending || _state.load() != ConnectionState::Active) {
        return;
    }
    _idleTimerPending = true;
    const auto generation = _idleTimerGeneration;
    const auto delay = time::TimePoint::now().timeDeltaTo(_idleDeadline);
    const auto weakSelf =
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(delay, [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleIdleTimeout(generation);
        }
    });
}

void TlsServerConnection::handleHandshakeTimeout(const std::uint64_t generation, const time::TimePoint deadline) {
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

void TlsServerConnection::handleIdleTimeout(const std::uint64_t generation) {
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

void TlsServerConnection::handleCloseTimeout(const std::uint64_t generation, const time::TimePoint deadline) {
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

void TlsServerConnection::enterClosing(const ConnectionCloseOrigin origin) {
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
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.closeTimeout(), [weakSelf, generation, deadline]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleCloseTimeout(generation, deadline);
        }
    });
}

auto TlsServerConnection::protocolErrorContext() const -> NetworkErrorContext {
    auto context = NetworkErrorContext{
        "TLS server connection failed"_el,
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
    case TlsAlertDescription::AccessDenied:
    case TlsAlertDescription::InsufficientSecurity:
    case TlsAlertDescription::NoApplicationProtocol:
    case TlsAlertDescription::UnrecognizedName:
        return context.setReason(NetworkErrorReason::TlsPolicyFailure);
    case TlsAlertDescription::InternalError:
        return context.setReason(NetworkErrorReason::TlsInternalFailure);
    default:
        return context.setReason(NetworkErrorReason::TlsProtocolFailure);
    }
}

auto TlsServerConnection::enrichTransportError(NetworkErrorContext context) const -> NetworkErrorContext {
    auto phase = NetworkErrorPhase::Accepting;
    switch (_state.load()) {
    case ConnectionState::Inactive:
        phase = NetworkErrorPhase::Configuration;
        break;
    case ConnectionState::Accepting:
        phase = NetworkErrorPhase::Accepting;
        break;
    case ConnectionState::Connecting:
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
    if (!context.remoteEndpoint().has_value()) {
        if (const auto endpoint = _tcpConnection->remoteEndpoint(); endpoint.has_value()) {
            context.setRemoteEndpoint(HostEndpoint{endpoint->address(), endpoint->port(), endpoint->scopeId()});
        }
    }
    return context;
}

void TlsServerConnection::finishFailed(NetworkErrorContext context) {
    if (_finalized.load() || _state.load() == ConnectionState::Failed) {
        return;
    }
    _state.store(ConnectionState::Failed);
    ++_handshakeTimerGeneration;
    ++_idleTimerGeneration;
    ++_closeTimerGeneration;
    _idleTimerPending = false;
    _handshakeLease.release();
    if (_protocol != nullptr) {
        _protocol->abort();
    }
    try {
        _tcpConnection->abort();
    } catch (...) {}
    auto firstException = std::exception_ptr{};
    try {
        const auto callback = _onError;
        if (callback) {
            callback(context);
        }
    } catch (...) {
        firstException = std::current_exception();
    }
    try {
        finishFinal();
    } catch (...) {
        if (firstException == nullptr) {
            throw;
        }
    }
    if (firstException != nullptr) {
        std::rethrow_exception(firstException);
    }
}

void TlsServerConnection::finishClosed() {
    if (_finalized.load()) {
        return;
    }
    _state.store(ConnectionState::Closed);
    ++_closeTimerGeneration;
    const auto context = ConnectionCloseContext{_closeOrigin.value_or(ConnectionCloseOrigin::Remote)};
    auto firstException = std::exception_ptr{};
    try {
        const auto callback = _onClosed;
        if (callback) {
            callback(context);
        }
    } catch (...) {
        firstException = std::current_exception();
    }
    try {
        finishFinal();
    } catch (...) {
        if (firstException == nullptr) {
            throw;
        }
    }
    if (firstException != nullptr) {
        std::rethrow_exception(firstException);
    }
}

void TlsServerConnection::finishFinal() {
    if (_finalized.exchange(true)) {
        return;
    }
    _handshakeLease.release();
    const auto callback = _onFinal;
    if (callback) {
        callback();
    }
}

void TlsServerConnection::postAbortFinal() {
    const auto weakSelf =
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    try {
        ownerEvents()->invoke([weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                const auto lock = std::scoped_lock{self->_mutex};
                self->finishFinal();
            }
        });
    } catch (...) {}
}

void TlsServerConnection::postAdmissionFailure(NetworkErrorContext context) {
    const auto weakSelf =
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, context = std::move(context)]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            const auto lock = std::scoped_lock{self->_mutex};
            if (self->_finalized.load()) {
                return;
            }
            const auto callback = self->_onError;
            auto firstException = std::exception_ptr{};
            try {
                if (callback) {
                    callback(context);
                }
            } catch (...) {
                firstException = std::current_exception();
            }
            try {
                self->finishFinal();
            } catch (...) {
                if (firstException == nullptr) {
                    throw;
                }
            }
            if (firstException != nullptr) {
                std::rethrow_exception(firstException);
            }
        }
    });
}

}
