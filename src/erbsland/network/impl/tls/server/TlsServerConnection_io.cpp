// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerConnection.hpp"

#include "../../../../err/Exception.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerConnection::configureTcpEvents() {
    const auto weakSelf =
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    _tcpConnection->events()
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
        .onClosed([weakSelf](const ConnectionCloseContext &) -> void {
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

void TlsServerConnection::handleTransportConnected() {
    const auto lock = std::scoped_lock{_mutex};
    if (!canContinue() || _state.load() != ConnectionState::Accepting || _protocol == nullptr) {
        return;
    }
    _state.store(ConnectionState::Handshaking);
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
        std::weak_ptr<TlsServerConnection>{std::static_pointer_cast<TlsServerConnection>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.handshakeTimeout(), [weakSelf, generation, deadline]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleHandshakeTimeout(generation, deadline);
        }
    });
    assessProtocolState();
}

void TlsServerConnection::handleTransportData(mem::ByteBlock data) {
    const auto lock = std::scoped_lock{_mutex};
    const auto current = _state.load();
    if (!canContinue() || _protocol == nullptr ||
        (current != ConnectionState::Handshaking && current != ConnectionState::Active &&
            current != ConnectionState::Closing)) {
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

void TlsServerConnection::handleTransportWritable() {
    const auto lock = std::scoped_lock{_mutex};
    if (!canContinue()) {
        return;
    }
    pumpTransportOutput();
    assessProtocolState();
    if (_applicationBlocked && !_pendingTransportRecord.has_value() && _state.load() == ConnectionState::Active) {
        _applicationBlocked = false;
        const auto callback = _onWritable;
        try {
            if (callback) {
                callback();
            }
        } catch (...) {
            abort();
            throw;
        }
    }
}

void TlsServerConnection::handleTransportClosed() {
    const auto lock = std::scoped_lock{_mutex};
    if (_abortRequested.load() || _finalized.load()) {
        return;
    }
    if (_protocol == nullptr) {
        finishFailed(
            NetworkErrorContext{"TLS transport closed"_el, "The accepted TCP transport closed before TLS started."_el}
                .setReason(NetworkErrorReason::TlsTruncation)
                .setPhase(NetworkErrorPhase::Accepting));
        return;
    }
    _protocol->transportClosed();
    if (_protocol->state() == TlsServerProtocolState::Closed) {
        finishClosed();
    } else {
        finishFailed(protocolErrorContext());
    }
}

void TlsServerConnection::handleTransportError(const NetworkErrorContext &context) {
    const auto lock = std::scoped_lock{_mutex};
    if (!_abortRequested.load() && !_finalized.load()) {
        finishFailed(enrichTransportError(context));
    }
}

void TlsServerConnection::handleTransportFinal() {
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

void TlsServerConnection::serviceCheckpoints() {
    while (canContinue() && _protocol != nullptr && _protocol->hasCheckpoint()) {
        auto callback = NetworkEventFn{};
        switch (_protocol->checkpoint()) {
        case TlsServerProtocolCheckpoint::None:
            return;
        case TlsServerProtocolCheckpoint::ClientHello:
            callback = _onClientHello;
            break;
        case TlsServerProtocolCheckpoint::HandshakeCompleted:
            _state.store(ConnectionState::Active);
            ++_handshakeTimerGeneration;
            _handshakeLease.release();
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

void TlsServerConnection::pumpTransportOutput() {
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
                NetworkErrorContext{"TLS transport write failed"_el, "TCP rejected a complete TLS record."_el}
                    .setReason(NetworkErrorReason::SocketOperationFailed)
                    .setPhase(
                        _state.load() == ConnectionState::Closing ? NetworkErrorPhase::Closing
                                                                  : NetworkErrorPhase::Handshaking));
            return;
        }
        _pendingTransportRecord.reset();
    }
    if (_applicationBlocked && !_pendingTransportRecord.has_value() && _state.load() == ConnectionState::Active) {
        _applicationBlocked = false;
        const auto callback = _onWritable;
        try {
            if (callback) {
                callback();
            }
        } catch (...) {
            abort();
            throw;
        }
    }
}

void TlsServerConnection::drainApplicationInput() {
    if (_receivingPaused || _protocol == nullptr || _state.load() != ConnectionState::Active) {
        return;
    }
    while (canContinue() && !_receivingPaused) {
        auto data = _protocol->takeApplicationData();
        if (!data.has_value()) {
            break;
        }
        recordApplicationActivity();
        const auto callback = _onData;
        try {
            if (callback) {
                callback(std::move(*data));
            }
        } catch (...) {
            abort();
            throw;
        }
    }
}

void TlsServerConnection::assessProtocolState() {
    if (!canContinue() || _protocol == nullptr) {
        return;
    }
    if (_protocol->state() == TlsServerProtocolState::Failed) {
        finishFailed(protocolErrorContext());
        return;
    }
    if (_protocol->peerCloseNotifyReceived() && !_closeOrigin.has_value()) {
        enterClosing(ConnectionCloseOrigin::Remote);
    }
    if (_protocol->state() == TlsServerProtocolState::Closing && !_closeOrigin.has_value()) {
        enterClosing(ConnectionCloseOrigin::Remote);
    }
    if (_protocol->state() == TlsServerProtocolState::Closed && !_pendingTransportRecord.has_value() &&
        !_protocol->hasTransportOutput()) {
        finishTransportClosure();
    }
}

void TlsServerConnection::finishTransportClosure() {
    if (!_transportClosing) {
        _transportClosing = true;
        _tcpConnection->close();
    }
}

}
