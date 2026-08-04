// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UdpSocket.hpp"

#include "../source/NetworkError.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

UdpSocket::UdpSocket(
    event::EventsPtr ownerEvents, event::EventLoopDriverPtr driver, UdpSocketDeviceCreateFn createDevice) :
    network::UdpSocket{std::move(ownerEvents)}, _driver{std::move(driver)}, _createDevice{std::move(createDevice)} {
    if (_driver == nullptr) {
        throw err::ParameterError{"The UDP socket requires an event-loop driver."_el, "driver"_el};
    }
    if (!_createDevice) {
        throw err::ParameterError{"The UDP socket requires a native device factory."_el, "createDevice"_el};
    }
}

UdpSocket::~UdpSocket() {
    abort();
    cleanupDevice();
}

auto UdpSocket::localEndpoint() const -> std::optional<IpEndpoint> {
    const auto lock = std::scoped_lock{_dataMutex};
    return _localEndpoint;
}

auto UdpSocket::state() const noexcept -> NetworkSourceState {
    return _state.load();
}

void UdpSocket::start(IpEndpoint localEndpoint, UdpSocketOptions options) {
    verifyCurrentOwnerEvents();
    if (_started.load()) {
        throw err::LogicError{"A UDP socket can only be started once."_el};
    }
    validateStart(localEndpoint, options);
    if (_started.exchange(true)) {
        throw err::LogicError{"A UDP socket can only be started once."_el};
    }
    auto expectedState = NetworkSourceState::Inactive;
    if (!_state.compare_exchange_strong(expectedState, NetworkSourceState::Starting)) {
        throw err::LogicError{"A terminated UDP socket cannot be started."_el};
    }
    _options = options;
    const auto generation = _generation.fetch_add(1U) + 1U;

    auto device = UdpSocketDevicePtr{};
    try {
        device = _createDevice(_driver, createCallbacks(generation));
        const auto actualEndpoint = device->bind(localEndpoint, options.maximumDatagramSize());
        {
            const auto lock = std::scoped_lock{_dataMutex};
            if (!isCurrent(generation) || _state.load() != NetworkSourceState::Starting) {
                device->abort();
                return;
            }
            _localEndpoint = actualEndpoint;
            _device = std::move(device);
        }
        postBound(generation);
    } catch (const NetworkError &error) {
        postError(generation, error.context());
    } catch (const err::RuntimeError &error) {
        postError(
            generation,
            NetworkErrorContext{"UDP socket startup failed"_el, error.reason()}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setLocalEndpoint(std::move(localEndpoint)));
    }
}

auto UdpSocket::send(const UdpDatagram &datagram) -> NetworkSendStatus {
    verifyCurrentOwnerEvents();
    if (_state.load() != NetworkSourceState::Active) {
        return NetworkSendStatus::Closed;
    }
    validateDatagram(datagram);
    const auto lock = std::scoped_lock{_dataMutex};
    if (_state.load() != NetworkSourceState::Active) {
        return NetworkSendStatus::Closed;
    }
    const auto charge = queueCharge(datagram);
    if (_sendQueueSize + charge > _options.sendQueueLimit()) {
        _blockedCharge = std::max(_blockedCharge, charge);
        return NetworkSendStatus::WouldBlock;
    }
    _sendQueue.emplace_back(datagram);
    _sendQueueSize += charge;
    flushOutput();
    return NetworkSendStatus::Accepted;
}

void UdpSocket::pauseReceiving() {
    verifyCurrentOwnerEvents();
    if (_state.load() != NetworkSourceState::Active || _receivePaused) {
        return;
    }
    _receivePaused = true;
    try {
        if (_device != nullptr) {
            _device->setReceiving(false);
        }
    } catch (const NetworkError &error) {
        postError(_generation.load(), error.context());
    }
}

void UdpSocket::resumeReceiving() {
    verifyCurrentOwnerEvents();
    if (_state.load() != NetworkSourceState::Active || !_receivePaused) {
        return;
    }
    _receivePaused = false;
    try {
        if (_device != nullptr) {
            _device->setReceiving(true);
        }
    } catch (const NetworkError &error) {
        postError(_generation.load(), error.context());
    }
}

void UdpSocket::close() {
    verifyCurrentOwnerEvents();
    const auto currentState = _state.load();
    if (currentState == NetworkSourceState::Closed || currentState == NetworkSourceState::Failed ||
        currentState == NetworkSourceState::Closing) {
        return;
    }
    if (!_started.exchange(true)) {
        const auto generation = _generation.fetch_add(1U) + 1U;
        _state.store(NetworkSourceState::Closed);
        postClosed(generation);
        return;
    }
    auto expectedState = currentState;
    if (!_state.compare_exchange_strong(expectedState, NetworkSourceState::Closing)) {
        return;
    }
    _receivePaused = true;
    try {
        if (_device != nullptr) {
            _device->setReceiving(false);
        }
    } catch (const NetworkError &error) {
        postError(_generation.load(), error.context());
        return;
    }
    if (_sendQueue.empty() && !_sendPending) {
        finishClose();
    }
}

void UdpSocket::abort() noexcept {
    _started.store(true);
    const auto previous = _state.exchange(NetworkSourceState::Closed);
    _generation.fetch_add(1U);
    if (previous == NetworkSourceState::Closed || previous == NetworkSourceState::Failed) {
        return;
    }
    const auto lock = std::scoped_lock{_dataMutex};
    if (_device != nullptr) {
        _device->abort();
    }
}

auto UdpSocket::events() -> network::UdpSocketEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<impl::UdpSocketEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

auto UdpSocket::createCallbacks(const std::uint64_t generation) -> UdpSocketDeviceCallbacks {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    return UdpSocketDeviceCallbacks{
        .datagram = [weakSelf, generation](UdpDatagram datagram) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postDatagram(generation, std::move(datagram));
            }
        },
        .datagramDropped = [weakSelf, generation](UdpDatagramDropContext context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postDatagramDropped(generation, std::move(context));
            }
        },
        .sendCompleted = [weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postSendCompleted(generation);
            }
        },
        .writable = [weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postWritable(generation);
            }
        },
        .error = [weakSelf, generation](NetworkErrorContext context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postError(generation, std::move(context));
            }
        },
    };
}

void UdpSocket::postBound(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverBound(generation);
        }
    });
}

void UdpSocket::postDatagram(const std::uint64_t generation, UdpDatagram datagram) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, datagram = std::move(datagram)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverDatagram(generation, std::move(datagram));
        }
    });
}

void UdpSocket::postDatagramDropped(const std::uint64_t generation, UdpDatagramDropContext context) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, context = std::move(context)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverDatagramDropped(generation, std::move(context));
        }
    });
}

void UdpSocket::postSendCompleted(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverSendCompleted(generation);
        }
    });
}

void UdpSocket::postWritable(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverNativeWritable(generation);
        }
    });
}

void UdpSocket::postClosed(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverClosed(generation);
        }
    });
}

void UdpSocket::postError(const std::uint64_t generation, NetworkErrorContext context) {
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation, context = std::move(context)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverError(generation, std::move(context));
        }
    });
}

void UdpSocket::deliverBound(const std::uint64_t generation) {
    if (!isCurrent(generation)) {
        return;
    }
    auto expectedState = NetworkSourceState::Starting;
    if (!_state.compare_exchange_strong(expectedState, NetworkSourceState::Active)) {
        return;
    }
    try {
        if (_device != nullptr) {
            _device->setReceiving(true);
        }
    } catch (const NetworkError &error) {
        deliverError(generation, error.context());
        return;
    }
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Active) {
        return;
    }
    const auto callback = _onBound;
    if (callback) {
        callback();
    }
}

void UdpSocket::deliverDatagram(const std::uint64_t generation, UdpDatagram datagram) {
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Active || _receivePaused) {
        return;
    }
    const auto callback = _onDatagram;
    if (callback) {
        callback(std::move(datagram));
    }
}

void UdpSocket::deliverDatagramDropped(const std::uint64_t generation, UdpDatagramDropContext context) {
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Active || _receivePaused) {
        return;
    }
    const auto callback = _onDatagramDropped;
    if (callback) {
        callback(context);
    }
}

void UdpSocket::deliverSendCompleted(const std::uint64_t generation) {
    if (!isCurrent(generation) || !_sendPending ||
        (_state.load() != NetworkSourceState::Active && _state.load() != NetworkSourceState::Closing)) {
        return;
    }
    _sendPending = false;
    finishAcceptedDatagram();
    flushOutput();
}

void UdpSocket::deliverNativeWritable(const std::uint64_t generation) {
    if (!isCurrent(generation) || !_nativeWriteBlocked ||
        (_state.load() != NetworkSourceState::Active && _state.load() != NetworkSourceState::Closing)) {
        return;
    }
    _nativeWriteBlocked = false;
    flushOutput();
}

void UdpSocket::deliverClosed(const std::uint64_t generation) {
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Closed) {
        return;
    }
    const auto callback = _onClosed;
    if (callback) {
        callback();
    }
}

void UdpSocket::deliverError(const std::uint64_t generation, NetworkErrorContext context) {
    if (!isCurrent(generation)) {
        return;
    }
    auto expectedState = _state.load();
    while (
        expectedState != NetworkSourceState::Closed && expectedState != NetworkSourceState::Failed &&
        !_state.compare_exchange_weak(expectedState, NetworkSourceState::Failed)) {}
    if (expectedState == NetworkSourceState::Closed || expectedState == NetworkSourceState::Failed) {
        return;
    }
    cleanupDevice();
    _sendQueue.clear();
    _sendQueueSize = {};
    _sendPending = false;
    _nativeWriteBlocked = false;
    if (!isCurrent(generation)) {
        return;
    }
    const auto callback = _onError;
    if (callback) {
        callback(context);
    }
}

void UdpSocket::flushOutput() {
    const auto currentState = _state.load();
    if ((currentState != NetworkSourceState::Active && currentState != NetworkSourceState::Closing) || _sendPending ||
        _nativeWriteBlocked || _device == nullptr) {
        return;
    }
    while (!_sendQueue.empty()) {
        try {
            const auto status = _device->send(_sendQueue.front());
            if (status == UdpSocketDeviceSendStatus::Pending) {
                _sendPending = true;
                return;
            }
            if (status == UdpSocketDeviceSendStatus::WouldBlock) {
                _nativeWriteBlocked = true;
                return;
            }
        } catch (const NetworkError &error) {
            postError(_generation.load(), error.context());
            return;
        }
        finishAcceptedDatagram();
    }
    if (_state.load() == NetworkSourceState::Closing) {
        finishClose();
    }
}

void UdpSocket::finishAcceptedDatagram() {
    if (_sendQueue.empty()) {
        return;
    }
    _sendQueueSize -= queueCharge(_sendQueue.front());
    _sendQueue.pop_front();
    notifyWritableIfReady();
}

void UdpSocket::notifyWritableIfReady() {
    if (_state.load() != NetworkSourceState::Active || _blockedCharge.isZero()) {
        return;
    }
    const auto available = _options.sendQueueLimit() - _sendQueueSize;
    if (available < _blockedCharge) {
        return;
    }
    _blockedCharge = {};
    const auto generation = _generation.load();
    const auto weakSelf = std::weak_ptr<UdpSocket>{std::static_pointer_cast<UdpSocket>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock();
            self != nullptr && self->isCurrent(generation) && self->_state.load() == NetworkSourceState::Active) {
            const auto callback = self->_onWritable;
            if (callback) {
                callback();
            }
        }
    });
}

void UdpSocket::finishClose() {
    cleanupDevice();
    _state.store(NetworkSourceState::Closed);
    postClosed(_generation.load());
}

void UdpSocket::cleanupDevice() noexcept {
    const auto lock = std::scoped_lock{_dataMutex};
    if (_device != nullptr) {
        _device->close();
        _device.reset();
    }
}

void UdpSocket::validateStart(const IpEndpoint &localEndpoint, const UdpSocketOptions &options) const {
    if (localEndpoint.address().isV4() && localEndpoint.scopeId().isSpecified()) {
        throw err::ParameterError{"An IPv4 local endpoint cannot contain an IPv6 scope."_el, "localEndpoint"_el};
    }
    if (!options.maximumDatagramSize().isFinite() || options.maximumDatagramSize().isZero() ||
        options.maximumDatagramSize() > UdpSocketOptions::cMaximumPortableDatagramSize) {
        throw err::ParameterError{
            "The maximum UDP datagram size must be positive, finite, and portable."_el,
            "options.maximumDatagramSize"_el};
    }
    if (!options.sendQueueLimit().isFinite() || options.sendQueueLimit().isZero()) {
        throw err::ParameterError{
            "The UDP send queue limit must be positive and finite."_el, "options.sendQueueLimit"_el};
    }
}

void UdpSocket::validateDatagram(const UdpDatagram &datagram) const {
    if (datagram.remoteEndpoint().port().isAutomatic()) {
        throw err::ParameterError{"A UDP destination port must not be automatic."_el, "datagram.remoteEndpoint"_el};
    }
    if (datagram.remoteEndpoint().address().isV4() && datagram.remoteEndpoint().scopeId().isSpecified()) {
        throw err::ParameterError{"An IPv4 destination cannot contain an IPv6 scope."_el, "datagram.remoteEndpoint"_el};
    }
    const auto local = localEndpoint();
    if (!local.has_value() || local->address().version() != datagram.remoteEndpoint().address().version()) {
        throw err::ParameterError{
            "The UDP destination address family must match the bound socket."_el, "datagram.remoteEndpoint"_el};
    }
    const auto length = datagram.data().length();
    if (length > _options.maximumDatagramSize()) {
        throw err::ParameterError{
            "The UDP payload exceeds the configured maximum datagram size."_el, "datagram.data"_el};
    }
    if (queueCharge(datagram) > _options.sendQueueLimit()) {
        throw err::ParameterError{"The UDP payload exceeds the configured send queue limit."_el, "datagram.data"_el};
    }
}

auto UdpSocket::queueCharge(const UdpDatagram &datagram) noexcept -> unit::ByteLength {
    return datagram.data().length().isZero() ? unit::ByteLength::one() : datagram.data().length();
}

auto UdpSocket::isCurrent(const std::uint64_t generation) const noexcept -> bool {
    return _generation.load() == generation;
}

}
