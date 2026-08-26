// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UdpSocket.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../event/Events.hpp"
#include "../../../text/Literals.hpp"
#include "../../source/NetworkError.hpp"

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

}
