// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientConnection.hpp"

#include "../../err/Exception.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsClientConnection::configureTcpEvents() {
    const auto weakSelf =
        std::weak_ptr<TlsClientConnection>{std::static_pointer_cast<TlsClientConnection>(shared_from_this())};
    _tcpConnection->events()
        .onHostResolved([weakSelf](const util::List<IpEndpoint> &endpoints) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleHostResolved(endpoints);
            }
        })
        .onConnected([weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportConnected();
            }
        })
        .onData([weakSelf](mem::ByteBlock data) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportData(std::move(data));
            }
        })
        .onWritable([weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportWritable();
            }
        })
        .onClosed([weakSelf](const TcpConnectionCloseContext &) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportClosed();
            }
        })
        .onError([weakSelf](const NetworkErrorContext &context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportError(context);
            }
        })
        .onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleTransportFinal();
            }
        });
}

void TlsClientConnection::handleHostResolved(const util::List<IpEndpoint> &endpoints) {
    const auto lock = std::scoped_lock{_mutex};
    if (!canContinue() || _state.load() != TlsClientConnectionState::Connecting) {
        return;
    }
    _hostResolved = true;
    try {
        const auto callback = _onHostResolved;
        if (callback) {
            callback(endpoints);
        }
    } catch (...) {
        abort();
        throw;
    }
}

void TlsClientConnection::handleTransportConnected() {
    const auto lock = std::scoped_lock{_mutex};
    if (!canContinue() || _state.load() != TlsClientConnectionState::Connecting || _protocol == nullptr) {
        return;
    }
    _state.store(TlsClientConnectionState::Handshaking);
    try {
        const auto callback = _onTransportConnected;
        if (callback) {
            callback();
        }
    } catch (...) {
        abort();
        throw;
    }
    if (!canContinue()) {
        return;
    }
    try {
        _startProtocol(*_protocol);
    } catch (const err::Exception &error) {
        finishFailed(
            NetworkErrorContext{"TLS handshake failed"_el, error.reason()}
                .setReason(NetworkErrorReason::TlsInternalFailure)
                .setPhase(NetworkErrorPhase::Handshaking));
        return;
    } catch (...) {
        finishFailed(
            NetworkErrorContext{"TLS handshake failed"_el, "TLS protocol initialization failed."_el}
                .setReason(NetworkErrorReason::TlsInternalFailure)
                .setPhase(NetworkErrorPhase::Handshaking));
        return;
    }
    const auto generation = ++_handshakeTimerGeneration;
    const auto deadline = time::TimePoint::now() + _options.handshakeTimeout();
    const auto weakSelf =
        std::weak_ptr<TlsClientConnection>{std::static_pointer_cast<TlsClientConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.handshakeTimeout(), [weakSelf, generation, deadline]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleHandshakeTimeout(generation, deadline);
        }
    });
    pumpTransportOutput();
    assessProtocolState();
}

void TlsClientConnection::handleTransportData(mem::ByteBlock data) {
    const auto lock = std::scoped_lock{_mutex};
    const auto current = _state.load();
    if (!canContinue() || _protocol == nullptr ||
        (current != TlsClientConnectionState::Handshaking && current != TlsClientConnectionState::Active &&
            current != TlsClientConnectionState::Closing)) {
        return;
    }
    _protocol->feedTransport(data.span());
    serviceCheckpoints();
    if (!canContinue()) {
        return;
    }
    pumpTransportOutput();
    drainApplicationInput();
    assessProtocolState();
}

void TlsClientConnection::handleTransportWritable() {
    const auto lock = std::scoped_lock{_mutex};
    if (!canContinue()) {
        return;
    }
    pumpTransportOutput();
    assessProtocolState();
    if (_applicationBlocked && _pendingTransportRecord == std::nullopt &&
        _state.load() == TlsClientConnectionState::Active) {
        _applicationBlocked = false;
        const auto callback = _onWritable;
        if (callback) {
            callback();
        }
    }
}

void TlsClientConnection::handleTransportClosed() {
    const auto lock = std::scoped_lock{_mutex};
    if (_abortRequested.load() || _finalized.load()) {
        return;
    }
    if (_protocol == nullptr) {
        finishFailed(
            NetworkErrorContext{"TLS transport closed"_el, "The TCP transport closed before TLS started."_el}
                .setReason(NetworkErrorReason::TlsTruncation)
                .setPhase(NetworkErrorPhase::Connecting));
        return;
    }
    _protocol->transportClosed();
    if (_protocol->state() == TlsClientProtocolState::Closed) {
        finishClosed();
    } else {
        finishFailed(protocolErrorContext());
    }
}

void TlsClientConnection::handleTransportError(const NetworkErrorContext &context) {
    const auto lock = std::scoped_lock{_mutex};
    if (_abortRequested.load() || _finalized.load()) {
        return;
    }
    finishFailed(enrichTransportError(context));
}

void TlsClientConnection::handleTransportFinal() {
    const auto lock = std::scoped_lock{_mutex};
    if (_abortRequested.load()) {
        if (_protocol != nullptr) {
            _protocol->abort();
        }
        finishFinal();
        return;
    }
    if (!_finalized.load()) {
        finishFailed(
            NetworkErrorContext{"TLS transport ended"_el, "The TCP transport finalized without a TLS outcome."_el}
                .setReason(NetworkErrorReason::TlsInternalFailure)
                .setPhase(NetworkErrorPhase::Closing));
    }
}

void TlsClientConnection::serviceCheckpoints() {
    while (canContinue() && _protocol != nullptr && _protocol->hasCheckpoint()) {
        auto callback = NetworkEventFn{};
        switch (_protocol->checkpoint()) {
        case TlsClientProtocolCheckpoint::None:
            return;
        case TlsClientProtocolCheckpoint::PeerHello:
            callback = _onPeerHello;
            break;
        case TlsClientProtocolCheckpoint::PeerAuthenticated:
            callback = _onPeerAuthenticated;
            break;
        case TlsClientProtocolCheckpoint::HandshakeCompleted:
            _state.store(TlsClientConnectionState::Active);
            ++_handshakeTimerGeneration;
            recordApplicationActivity();
            callback = _onHandshakeCompleted;
            break;
        }
        try {
            if (callback) {
                callback();
            }
        } catch (...) {
            abort();
            throw;
        }
        if (!canContinue()) {
            return;
        }
        _protocol->resume();
    }
}

void TlsClientConnection::pumpTransportOutput() {
    if (!canContinue() || _protocol == nullptr || _protocol->hasCheckpoint()) {
        return;
    }
    while (canContinue()) {
        if (!_pendingTransportRecord.has_value()) {
            _pendingTransportRecord = _protocol->takeTransportOutput();
            if (!_pendingTransportRecord.has_value()) {
                break;
            }
        }
        const auto status = _tcpConnection->send(*_pendingTransportRecord);
        if (status == NetworkSendStatus::WouldBlock) {
            return;
        }
        if (status == NetworkSendStatus::Closed) {
            finishFailed(
                NetworkErrorContext{"TLS transport write failed"_el, "The TCP transport rejected a TLS record."_el}
                    .setReason(NetworkErrorReason::SocketOperationFailed)
                    .setPhase(
                        _state.load() == TlsClientConnectionState::Closing ? NetworkErrorPhase::Closing
                                                                           : NetworkErrorPhase::Handshaking));
            return;
        }
        _pendingTransportRecord.reset();
    }
    if (_applicationBlocked && _pendingTransportRecord == std::nullopt &&
        _state.load() == TlsClientConnectionState::Active) {
        _applicationBlocked = false;
        const auto callback = _onWritable;
        if (callback) {
            callback();
        }
    }
}

void TlsClientConnection::drainApplicationInput() {
    if (_receivingPaused || _protocol == nullptr || _state.load() != TlsClientConnectionState::Active) {
        return;
    }
    while (canContinue() && !_receivingPaused) {
        auto data = _protocol->takeApplicationData();
        if (!data.has_value()) {
            break;
        }
        recordApplicationActivity();
        const auto callback = _onData;
        if (callback) {
            callback(std::move(*data));
        }
    }
}

void TlsClientConnection::assessProtocolState() {
    if (!canContinue() || _protocol == nullptr) {
        return;
    }
    if (_protocol->state() == TlsClientProtocolState::Failed) {
        finishFailed(protocolErrorContext());
        return;
    }
    if (_protocol->peerCloseNotifyReceived() && !_closeOrigin.has_value()) {
        enterClosing(TlsClientConnectionCloseOrigin::Remote);
    }
    if (_protocol->state() == TlsClientProtocolState::Closing && !_closeOrigin.has_value()) {
        enterClosing(TlsClientConnectionCloseOrigin::Remote);
    }
    if (_protocol->state() == TlsClientProtocolState::Closed && !_pendingTransportRecord.has_value() &&
        !_protocol->hasTransportOutput()) {
        finishTransportClosure();
    }
}

void TlsClientConnection::finishTransportClosure() {
    if (_transportClosing) {
        return;
    }
    _transportClosing = true;
    _tcpConnection->close();
}

}
