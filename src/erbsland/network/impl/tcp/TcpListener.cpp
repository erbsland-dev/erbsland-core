// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpListener.hpp"

#include "TcpConnectionRequest.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../event/EventTarget.hpp"
#include "../../../text/Literals.hpp"
#include "../../source/NetworkError.hpp"

#include <exception>
#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace text::literals;

TcpListener::TcpListener(
    event::EventTargetPtr ownerLoop, event::EventLoopDriverPtr driver, TcpListenerDeviceCreateFn createDevice) :
    network::TcpListener{std::move(ownerLoop)}, _driver{std::move(driver)}, _createDevice{std::move(createDevice)} {
    if (_driver == nullptr || !_createDevice) {
        throw LogicFailure{"A TCP listener requires a driver and device factory."_el};
    }
}

TcpListener::~TcpListener() {
    _quotaSubscription.cancel();
    cleanupDevice(true);
}

auto TcpListener::localEndpoint() const -> std::optional<IpEndpoint> {
    const auto lock = std::scoped_lock{_dataMutex};
    return _localEndpoint;
}

auto TcpListener::state() const noexcept -> NetworkSourceState {
    return _state.load();
}

void TcpListener::start(IpEndpoint localEndpoint, TcpListenerOptions options) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    if (_started.load()) {
        throw LogicFailure{"A TCP listener can only be started once."_el};
    }
    validateStart(localEndpoint, options);
    if (_started.exchange(true)) {
        throw LogicFailure{"A TCP listener can only be started once."_el};
    }
    _state.store(NetworkSourceState::Starting);
    if (!options._hasExplicitConnectionQuota) {
        options._connectionQuota = ConnectionQuota::create(TcpListenerOptions::cDefaultMaximumConnections);
    }
    _options = std::move(options);
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    _quotaSubscription = _options.connectionQuota()->addCapacityChanged([weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->handleQuotaCapacity();
        }
    });
    const auto generation = _generation.fetch_add(1U) + 1U;
    try {
        auto device = _createDevice(_driver, createCallbacks(generation));
        const auto actualEndpoint = device->start(localEndpoint, _options.backlog());
        {
            const auto lock = std::scoped_lock{_dataMutex};
            _localEndpoint = actualEndpoint;
            _device = std::move(device);
        }
        postListening(generation);
    } catch (const NetworkError &error) {
        postError(generation, error.context());
    } catch (const RuntimeError &error) {
        postError(
            generation,
            NetworkErrorContext{"TCP listener startup failed"_el, error.reason()}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setLocalEndpoint(std::move(localEndpoint)));
    }
}

void TcpListener::pauseAccepting() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != NetworkSourceState::Active || _acceptPaused) {
        return;
    }
    _acceptPaused = true;
    updateAccepting();
}

void TcpListener::resumeAccepting() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    if (_state.load() != NetworkSourceState::Active || !_acceptPaused) {
        return;
    }
    _acceptPaused = false;
    updateAccepting();
}

void TcpListener::close() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    const auto current = _state.exchange(NetworkSourceState::Closed);
    if (current == NetworkSourceState::Closed || current == NetworkSourceState::Failed) {
        return;
    }
    _started.store(true);
    cleanupDevice(false);
    postClosedAndFinal(_generation.load());
}

void TcpListener::abort() noexcept {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    _started.store(true);
    const auto previous = _state.exchange(NetworkSourceState::Closed);
    if (previous == NetworkSourceState::Closed || previous == NetworkSourceState::Failed) {
        return;
    }
    const auto generation = _generation.fetch_add(1U) + 1U;
    cleanupDevice(true);
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->postFinal(generation);
        }
    });
}

auto TcpListener::events() -> network::TcpListenerEventEditor & {
    auto target = currentOwnerLoop();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<TcpListenerEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

auto TcpListener::createCallbacks(const std::uint64_t generation) -> TcpListenerDeviceCallbacks {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    return TcpListenerDeviceCallbacks{
        .accepted = [weakSelf, generation](TcpAcceptedSocketPtr socket) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->handleAccepted(generation, std::move(socket));
            }
        },
        .error = [weakSelf, generation](NetworkErrorContext context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postError(generation, std::move(context));
            }
        },
    };
}

void TcpListener::handleAccepted(const std::uint64_t generation, TcpAcceptedSocketPtr socket) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Active || socket == nullptr) {
        return;
    }
    const auto &filter = _options.connectionFilter();
    if (filter && filter(socket->remoteEndpoint()) == TcpConnectionFilterResult::Reject) {
        return;
    }
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Active) {
        return;
    }
    auto quotaLease = _options.connectionQuota()->tryAcquire(socket->remoteEndpoint());
    if (!quotaLease.has_value()) {
        updateAccepting();
        return;
    }
    ++_pendingRequestCount;
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    auto request =
        std::make_shared<TcpConnectionRequest>(std::move(socket), std::move(*quotaLease), [weakSelf]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->ownerLoop()->invoke([weakSelf]() -> void {
                    if (const auto target = weakSelf.lock(); target != nullptr) {
                        target->releasePendingRequest();
                    }
                });
            }
        });
    updateAccepting();
    const auto callback = _onConnection;
    ownerLoop()->invoke([callback, request = std::move(request)]() -> void {
        if (callback) {
            callback(request);
        }
    });
}

void TcpListener::postListening(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverListening(generation);
        }
    });
}

void TcpListener::postError(const std::uint64_t generation, NetworkErrorContext context) {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf, generation, context = std::move(context)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverError(generation, std::move(context));
        }
    });
}

void TcpListener::deliverListening(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || _state.load() != NetworkSourceState::Starting) {
        return;
    }
    _state.store(NetworkSourceState::Active);
    updateAccepting();
    const auto callback = _onListening;
    if (callback) {
        callback();
    }
}

void TcpListener::deliverError(const std::uint64_t generation, NetworkErrorContext context) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation)) {
        return;
    }
    const auto previous = _state.exchange(NetworkSourceState::Failed);
    if (previous == NetworkSourceState::Closed || previous == NetworkSourceState::Failed) {
        return;
    }
    cleanupDevice(true);
    postErrorAndFinal(generation, std::move(context));
}

void TcpListener::releasePendingRequest() {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (_pendingRequestCount > 0U) {
        --_pendingRequestCount;
    }
    updateAccepting();
}

void TcpListener::handleQuotaCapacity() {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            const auto lifecycleLock = std::scoped_lock{self->_lifecycleMutex};
            self->updateAccepting();
        }
    });
}

void TcpListener::updateAccepting() {
    const auto enabled = _state.load() == NetworkSourceState::Active && !_acceptPaused &&
        _pendingRequestCount < _options.maximumPendingRequests().toSizeT() &&
        !_options.connectionQuota()->available().isZero();
    try {
        if (const auto device = getDevice(); device != nullptr) {
            device->setAccepting(enabled);
        }
    } catch (const NetworkError &error) {
        postError(_generation.load(), error.context());
    }
}

void TcpListener::postClosedAndFinal(const std::uint64_t generation) {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf, generation]() -> void {
        const auto self = weakSelf.lock();
        if (self == nullptr || !self->isCurrent(generation) || self->_finalPosted.exchange(true)) {
            return;
        }
        auto firstException = std::exception_ptr{};
        try {
            const auto callback = self->_onClosed;
            if (callback) {
                callback();
            }
        } catch (...) {
            firstException = std::current_exception();
        }
        try {
            const auto callback = self->_onFinal;
            if (callback) {
                callback();
            }
        } catch (const RuntimeError &) {
            if (firstException == nullptr) {
                throw;
            }
        }
        if (firstException != nullptr) {
            std::rethrow_exception(firstException);
        }
    });
}

void TcpListener::postErrorAndFinal(std::uint64_t generation, NetworkErrorContext context) {
    const auto weakSelf = std::weak_ptr<TcpListener>{std::static_pointer_cast<TcpListener>(shared_from_this())};
    ownerLoop()->invoke([weakSelf, generation, context = std::move(context)]() -> void {
        const auto self = weakSelf.lock();
        if (self == nullptr || !self->isCurrent(generation) || self->_finalPosted.exchange(true)) {
            return;
        }
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
        } catch (const RuntimeError &) {
            if (firstException == nullptr) {
                throw;
            }
        }
        if (firstException != nullptr) {
            std::rethrow_exception(firstException);
        }
    });
}

void TcpListener::postFinal(const std::uint64_t generation) {
    if (!isCurrent(generation) || _finalPosted.exchange(true)) {
        return;
    }
    const auto callback = _onFinal;
    if (callback) {
        callback();
    }
}

auto TcpListener::getDevice() const -> TcpListenerDevicePtr {
    const auto lock = std::scoped_lock{_dataMutex};
    return _device;
}

void TcpListener::cleanupDevice(const bool abortDevice) noexcept {
    auto device = TcpListenerDevicePtr{};
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

void TcpListener::validateStart(const IpEndpoint &localEndpoint, const TcpListenerOptions &options) const {
    if (localEndpoint.address().isV4() && localEndpoint.scopeId().isSpecified()) {
        throw ParameterFailure{"An IPv4 listener endpoint cannot contain an IPv6 scope."_el, "localEndpoint"_el};
    }
    if (!options.backlog().isFinite() || options.backlog().isZero()) {
        throw ParameterFailure{"The TCP listener backlog must be positive and finite."_el, "options.backlog"_el};
    }
    if (!options.maximumPendingRequests().isFinite() || options.maximumPendingRequests().isZero()) {
        throw ParameterFailure{
            "The TCP pending-request limit must be positive and finite."_el, "options.maximumPendingRequests"_el};
    }
    if (options.connectionQuota() == nullptr) {
        throw ParameterFailure{"The TCP listener connection quota must not be empty."_el, "options"_el};
    }
}

auto TcpListener::isCurrent(const std::uint64_t generation) const noexcept -> bool {
    return _generation.load() == generation;
}

}
