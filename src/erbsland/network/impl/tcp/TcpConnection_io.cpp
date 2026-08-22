// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpConnection.hpp"

#include "../host/HostLookup.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../event/Events.hpp"
#include "../../../text/Literals.hpp"
#include "../../source/NetworkError.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

auto TcpConnection::send(const mem::ByteBlock &data) -> NetworkSendStatus {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != ConnectionState::Active) {
        return NetworkSendStatus::Closed;
    }
    if (data.length() > _bufferLimits.send()) {
        throw err::ParameterError{"The TCP output block exceeds the configured send queue limit."_el, "data"_el};
    }
    if (data.isEmpty()) {
        return NetworkSendStatus::Accepted;
    }
    if (_sendQueueSize + data.length() > _bufferLimits.send()) {
        _blockedCharge = std::max(_blockedCharge, data.length());
        return NetworkSendStatus::WouldBlock;
    }
    _sendQueue.emplace_back(data);
    _sendQueueSize += data.length();
    flushOutput();
    return NetworkSendStatus::Accepted;
}

void TcpConnection::pauseReceiving() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != ConnectionState::Active || _receivePaused) {
        return;
    }
    _receivePaused = true;
    updateReceiving();
}

void TcpConnection::resumeReceiving() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerEvents();
    if (_state.load() != ConnectionState::Active || !_receivePaused) {
        return;
    }
    _receivePaused = false;
    scheduleDataDelivery();
    updateReceiving();
}

void TcpConnection::close() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerEvents();
    const auto current = _state.load();
    if (current == ConnectionState::Closed || current == ConnectionState::Failed ||
        current == ConnectionState::Closing) {
        return;
    }
    _started.store(true);
    _closeOrigin = ConnectionCloseOrigin::Local;
    _state.store(ConnectionState::Closing);
    if (_lookup != nullptr) {
        _lookup->cancel();
        _lookup.reset();
    }
    updateReceiving();
    scheduleDataDelivery();
    if (current == ConnectionState::Connecting || current == ConnectionState::Accepting) {
        _sendQueue.clear();
        _sendQueueSize = {};
        _receiveQueue.clear();
        _receiveQueueSize = {};
        finishClosed(ConnectionCloseOrigin::Local);
        return;
    }
    flushOutput();
    finishCloseIfReady();
}

void TcpConnection::abort() noexcept {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    _started.store(true);
    const auto previous = _state.exchange(ConnectionState::Closed);
    if (previous == ConnectionState::Closed || previous == ConnectionState::Failed) {
        return;
    }
    const auto generation = _generation.fetch_add(1U) + 1U;
    try {
        if (_lookup != nullptr) {
            _lookup->cancel();
        }
    } catch (...) {}
    cleanupDevice(true);
    try {
        const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
        ownerEvents()->invoke([weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->_sendQueue.clear();
                self->_sendQueueSize = {};
                self->_receiveQueue.clear();
                self->_receiveQueueSize = {};
                self->postFinal(generation);
            }
        });
    } catch (...) {}
}

void TcpConnection::postConnected(const std::uint64_t generation, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke(
        [weakSelf,
            generation,
            localEndpoint = std::move(localEndpoint),
            remoteEndpoint = std::move(remoteEndpoint)]() mutable -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->deliverConnected(generation, std::move(localEndpoint), std::move(remoteEndpoint));
            }
        });
}

void TcpConnection::postData(const std::uint64_t generation, mem::ByteBlock data) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || _state.load() != ConnectionState::Active || data.isEmpty()) {
        return;
    }
    _receiveQueueSize += data.length();
    _receiveQueue.emplace_back(std::move(data));
    updateReceiving();
    scheduleDataDelivery();
}

void TcpConnection::postSendCompleted(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverSendCompleted(generation);
        }
    });
}

void TcpConnection::postRemoteClosed(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverRemoteClosed(generation);
        }
    });
}

void TcpConnection::postError(const std::uint64_t generation, NetworkErrorContext context) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, context = std::move(context)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverError(generation, std::move(context));
        }
    });
}

void TcpConnection::deliverData(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    _receiveDeliveryPosted = false;
    const auto current = _state.load();
    if (!isCurrent(generation) || (current != ConnectionState::Active && current != ConnectionState::Closing) ||
        _receiveQueue.empty() || (_receivePaused && current == ConnectionState::Active)) {
        return;
    }
    auto data = std::move(_receiveQueue.front());
    _receiveQueue.pop_front();
    _receiveQueueSize -= data.length();
    try {
        const auto callback = _onData;
        if (callback) {
            callback(std::move(data));
        }
    } catch (...) {
        updateReceiving();
        scheduleDataDelivery();
        finishCloseIfReady();
        throw;
    }
    updateReceiving();
    scheduleDataDelivery();
    finishCloseIfReady();
}

void TcpConnection::deliverSendCompleted(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    const auto current = _state.load();
    if (!isCurrent(generation) || !_sendPending ||
        (current != ConnectionState::Active && current != ConnectionState::Closing)) {
        return;
    }
    _sendPending = false;
    finishAcceptedBlock();
    flushOutput();
    finishCloseIfReady();
}

void TcpConnection::deliverRemoteClosed(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || _state.load() != ConnectionState::Active) {
        return;
    }
    _closeOrigin = ConnectionCloseOrigin::Remote;
    _state.store(ConnectionState::Closing);
    updateReceiving();
    scheduleDataDelivery();
    flushOutput();
    finishCloseIfReady();
}

void TcpConnection::flushOutput() {
    const auto current = _state.load();
    if ((current != ConnectionState::Active && current != ConnectionState::Closing) || _sendPending) {
        return;
    }
    while (!_sendQueue.empty()) {
        try {
            auto status = TcpConnectionDeviceSendStatus::Pending;
            {
                const auto lock = std::scoped_lock{_dataMutex};
                if (_device == nullptr) {
                    return;
                }
                status = _device->send(_sendQueue.front());
            }
            if (status == TcpConnectionDeviceSendStatus::Pending) {
                _sendPending = true;
                return;
            }
        } catch (const NetworkError &error) {
            postError(_generation.load(), error.context());
            return;
        }
        finishAcceptedBlock();
    }
}

void TcpConnection::finishAcceptedBlock() {
    if (_sendQueue.empty()) {
        return;
    }
    _sendQueueSize -= _sendQueue.front().length();
    _sendQueue.pop_front();
    notifyWritableIfReady();
}

void TcpConnection::notifyWritableIfReady() {
    if (_state.load() != ConnectionState::Active || _blockedCharge.isZero()) {
        return;
    }
    if (_bufferLimits.send() - _sendQueueSize < _blockedCharge) {
        return;
    }
    _blockedCharge = {};
    const auto generation = _generation.load();
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock();
            self != nullptr && self->isCurrent(generation) && self->_state.load() == ConnectionState::Active) {
            const auto callback = self->_onWritable;
            if (callback) {
                callback();
            }
        }
    });
}

void TcpConnection::updateReceiving() {
    auto maximumBytes = unit::ByteLength{};
    if (_state.load() == ConnectionState::Active && !_receivePaused && _receiveQueueSize < _bufferLimits.receive()) {
        maximumBytes = _bufferLimits.receive() - _receiveQueueSize;
    }
    try {
        const auto lock = std::scoped_lock{_dataMutex};
        if (_device != nullptr) {
            _device->setReceiving(maximumBytes);
        }
    } catch (const NetworkError &error) {
        postError(_generation.load(), error.context());
    }
}

void TcpConnection::scheduleDataDelivery() {
    const auto current = _state.load();
    if (_receiveDeliveryPosted || _receiveQueue.empty() || (_receivePaused && current == ConnectionState::Active) ||
        (current != ConnectionState::Active && current != ConnectionState::Closing)) {
        return;
    }
    _receiveDeliveryPosted = true;
    const auto generation = _generation.load();
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverData(generation);
        }
    });
}

void TcpConnection::finishCloseIfReady() {
    if (_state.load() != ConnectionState::Closing || !_sendQueue.empty() || _sendPending || !_receiveQueue.empty() ||
        _receiveDeliveryPosted || !_closeOrigin.has_value()) {
        return;
    }
    finishClosed(*_closeOrigin);
}

void TcpConnection::finishClosed(const ConnectionCloseOrigin origin) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (_state.exchange(ConnectionState::Closed) == ConnectionState::Closed) {
        return;
    }
    cleanupDevice(false);
    postClosedAndFinal(_generation.load(), ConnectionCloseContext{origin});
}

void TcpConnection::finishFailed(NetworkErrorContext context) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    const auto previous = _state.exchange(ConnectionState::Failed);
    if (previous == ConnectionState::Closed || previous == ConnectionState::Failed) {
        return;
    }
    if (_lookup != nullptr) {
        _lookup->cancel();
        _lookup.reset();
    }
    cleanupDevice(true);
    _sendQueue.clear();
    _sendQueueSize = {};
    _receiveQueue.clear();
    _receiveQueueSize = {};
    postErrorAndFinal(_generation.load(), std::move(context));
}

void TcpConnection::postClosedAndFinal(const std::uint64_t generation, const ConnectionCloseContext context) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, context]() -> void {
        const auto self = weakSelf.lock();
        if (self == nullptr || !self->isCurrent(generation) || self->_finalPosted.exchange(true)) {
            return;
        }
        self->_quotaLease.release();
        auto firstException = std::exception_ptr{};
        try {
            const auto callback = self->_onClosed;
            if (callback) {
                callback(context);
            }
        } catch (...) {
            firstException = std::current_exception();
        }
        try {
            const auto callback = self->_onFinal;
            if (callback) {
                callback();
            }
        } catch (...) {
            if (firstException == nullptr) {
                throw;
            }
        }
        if (firstException != nullptr) {
            std::rethrow_exception(firstException);
        }
    });
}

void TcpConnection::postErrorAndFinal(std::uint64_t generation, NetworkErrorContext context) {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, context = std::move(context)]() -> void {
        const auto self = weakSelf.lock();
        if (self == nullptr || !self->isCurrent(generation) || self->_finalPosted.exchange(true)) {
            return;
        }
        self->_quotaLease.release();
        auto firstException = std::exception_ptr{};
        try {
            const auto callback = self->_onError;
            if (callback) {
                callback(context);
            }
        } catch (...) {
            firstException = std::current_exception();
        }
        try {
            const auto callback = self->_onFinal;
            if (callback) {
                callback();
            }
        } catch (...) {
            if (firstException == nullptr) {
                throw;
            }
        }
        if (firstException != nullptr) {
            std::rethrow_exception(firstException);
        }
    });
}

void TcpConnection::postFinal(const std::uint64_t generation) {
    if (!isCurrent(generation) || _finalPosted.exchange(true)) {
        return;
    }
    _quotaLease.release();
    const auto callback = _onFinal;
    if (callback) {
        callback();
    }
}

void TcpConnection::cleanupDevice(const bool abortDevice) noexcept {
    auto device = TcpConnectionDevicePtr{};
    {
        const auto lock = std::scoped_lock{_dataMutex};
        device = std::move(_device);
    }
    if (device != nullptr) {
        if (abortDevice) {
            device->abort();
        } else {
            device->close();
        }
    }
}

}
