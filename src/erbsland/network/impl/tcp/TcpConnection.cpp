// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TcpConnection.hpp"

#include "TcpConnectionRequest.hpp"

#include "../host/HostLookup.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../event/EventTarget.hpp"
#include "../../../event/impl/EventContinuation.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ItemIndex.hpp"
#include "../../source/NetworkError.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace text::literals;

TcpConnection::TcpConnection(
    event::EventTargetPtr ownerLoop,
    event::EventLoopDriverPtr driver,
    HostResolverPtr resolver,
    TcpConnectionDeviceCreateFn createDevice) :
    network::TcpConnection{std::move(ownerLoop)},
    _driver{std::move(driver)},
    _resolver{std::move(resolver)},
    _createDevice{std::move(createDevice)} {
    if (_driver == nullptr || _resolver == nullptr || !_createDevice) {
        throw LogicFailure{"A TCP connection requires a driver, resolver, and device factory."_el};
    }
    _abortContinuation = event::impl::EventContinuation::create(this->ownerLoop());
}

TcpConnection::~TcpConnection() {
    if (_lookup != nullptr) {
        _lookup->cancel();
    }
    cleanupDevice(true);
}

auto TcpConnection::localEndpoint() const -> std::optional<IpEndpoint> {
    const auto lock = std::scoped_lock{_dataMutex};
    return _localEndpoint;
}

auto TcpConnection::remoteEndpoint() const -> std::optional<IpEndpoint> {
    const auto lock = std::scoped_lock{_dataMutex};
    return _remoteEndpoint;
}

auto TcpConnection::bufferLimits() const noexcept -> SocketBufferLimits {
    return _bufferLimits;
}

auto TcpConnection::state() const noexcept -> ConnectionState {
    return _state.load();
}

void TcpConnection::connect(HostEndpoint remoteEndpoint, TcpConnectOptions options) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    if (_started.load()) {
        throw LogicFailure{"A TCP connection can only be started once."_el};
    }
    validateLimits(options.bufferLimits());
    if (!options.timeout().isPositive()) {
        throw ParameterFailure{"The TCP connection timeout must be positive."_el, "options.timeout"_el};
    }
    if (remoteEndpoint.port().isAutomatic()) {
        throw ParameterFailure{"A TCP destination port must not be automatic."_el, "remoteEndpoint"_el};
    }
    if (remoteEndpoint.host().isAddress() && remoteEndpoint.host().address()->isV4() &&
        remoteEndpoint.scopeId().isSpecified()) {
        throw ParameterFailure{"An IPv4 TCP destination cannot contain an IPv6 scope."_el, "remoteEndpoint"_el};
    }
    if (_started.exchange(true)) {
        throw LogicFailure{"A TCP connection can only be started once."_el};
    }
    _state.store(ConnectionState::Connecting);
    _requestedEndpoint = std::move(remoteEndpoint);
    _connectOptions = options;
    _bufferLimits = options.bufferLimits();
    _deadline = time::TimePoint::now() + options.timeout();
    const auto generation = _generation.fetch_add(1U) + 1U;
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    ownerLoop()->invokeAfter(options.timeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverTimeout(generation);
        }
    });
    startLookup(generation);
}

void TcpConnection::accept(network::TcpConnectionRequestPtr request, TcpAcceptOptions options) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    verifyCurrentOwnerLoop();
    if (_started.load()) {
        throw LogicFailure{"A TCP connection can only be started once."_el};
    }
    validateLimits(options.bufferLimits());
    if (request == nullptr) {
        throw ParameterFailure{"The TCP connection request must not be empty."_el, "request"_el};
    }
    const auto builtInRequest = std::dynamic_pointer_cast<impl::TcpConnectionRequest>(request);
    if (builtInRequest == nullptr) {
        throw ParameterFailure{
            "The TCP connection and request belong to incompatible network backends."_el, "request"_el};
    }
    auto device = _createDevice(_driver, options.bufferLimits().receive(), createCallbacks(_generation.load() + 1U));
    if (!builtInRequest->isCompatibleWith(*device)) {
        if (builtInRequest->state() != TcpConnectionRequestState::Pending) {
            throw LogicFailure{"The TCP connection request was already accepted or rejected."_el};
        }
        throw ParameterFailure{
            "The TCP connection and request use incompatible native socket backends."_el, "request"_el};
    }
    auto claim = builtInRequest->claim();
    if (!claim.has_value()) {
        throw LogicFailure{"The TCP connection request was accepted or rejected concurrently."_el};
    }
    _started.store(true);
    _state.store(ConnectionState::Accepting);
    _bufferLimits = options.bufferLimits();
    _quotaLease = std::move(claim->quotaLease);
    const auto generation = _generation.fetch_add(1U) + 1U;
    try {
        {
            const auto lock = std::scoped_lock{_dataMutex};
            _device = std::move(device);
        }
        const auto lock = std::scoped_lock{_dataMutex};
        _device->accept(std::move(claim->socket));
    } catch (const NetworkError &error) {
        postError(generation, error.context());
    } catch (const RuntimeError &error) {
        postError(
            generation,
            NetworkErrorContext{"TCP accept failed"_el, error.reason()}.setReason(
                NetworkErrorReason::SocketOperationFailed));
    }
}

auto TcpConnection::events() -> network::TcpConnectionEventEditor & {
    auto target = currentOwnerLoop();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<TcpConnectionEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

auto TcpConnection::createCallbacks(const std::uint64_t generation) -> TcpConnectionDeviceCallbacks {
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    return TcpConnectionDeviceCallbacks{
        .connected = [weakSelf, generation](IpEndpoint local, IpEndpoint remote) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postConnected(generation, std::move(local), std::move(remote));
            }
        },
        .data = [weakSelf, generation](mem::ByteBlock data) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postData(generation, std::move(data));
            }
        },
        .sendCompleted = [weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postSendCompleted(generation);
            }
        },
        .remoteClosed = [weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postRemoteClosed(generation);
            }
        },
        .error = [weakSelf, generation](NetworkErrorContext context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postError(generation, std::move(context));
            }
        },
    };
}

void TcpConnection::startLookup(const std::uint64_t generation) {
    auto lookup = std::make_shared<HostLookup>(ownerLoop(), _resolver);
    _lookup = lookup;
    const auto weakSelf = std::weak_ptr<TcpConnection>{std::static_pointer_cast<TcpConnection>(shared_from_this())};
    lookup->events()
        .onResolved([weakSelf, generation](const util::List<IpAddress> &addresses) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->deliverResolved(generation, addresses);
            }
        })
        .onError([weakSelf, generation](const NetworkErrorContext &context) -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                auto connectionContext = context;
                if (self->_requestedEndpoint.has_value()) {
                    connectionContext.setRemoteEndpoint(*self->_requestedEndpoint);
                }
                self->postError(generation, std::move(connectionContext));
            }
        })
        .onFinal([weakSelf, generation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr && self->isCurrent(generation)) {
                self->_lookup.reset();
            }
        });
    auto lookupOptions = _connectOptions.hostLookupOptions();
    const auto remaining = time::TimePoint::now().timeDeltaTo(_deadline);
    if (lookupOptions.timeout() > remaining) {
        lookupOptions.setTimeout(remaining);
    }
    lookup->start(_requestedEndpoint->host(), lookupOptions);
}

void TcpConnection::deliverResolved(const std::uint64_t generation, const util::List<IpAddress> &addresses) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || !isStarting()) {
        return;
    }
    auto endpoints = util::List<IpEndpoint>{};
    for (const auto &address : addresses) {
        const auto scope = address.isV6() ? _requestedEndpoint->scopeId() : ScopeId{};
        endpoints.append(IpEndpoint{address, _requestedEndpoint->port(), scope});
    }
    if (endpoints.isEmpty()) {
        postError(
            generation,
            NetworkErrorContext{"TCP connection failed"_el, "Host resolution produced no usable IP endpoints."_el}
                .setReason(NetworkErrorReason::NoAddresses)
                .setRemoteEndpoint(*_requestedEndpoint));
        return;
    }
    try {
        const auto callback = _onHostResolved;
        if (callback) {
            callback(endpoints);
        }
    } catch (...) {
        abort();
        throw;
    }
    if (!isCurrent(generation) || !isStarting()) {
        return;
    }
    _resolvedEndpoints = std::move(endpoints);
    _nextEndpoint = 0U;
    startNextEndpoint(generation);
}

void TcpConnection::startNextEndpoint(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || !isStarting()) {
        return;
    }
    if (time::TimePoint::now() >= _deadline) {
        deliverTimeout(generation);
        return;
    }
    if (_nextEndpoint >= _resolvedEndpoints.count().toSizeT()) {
        auto context = _lastConnectError.value_or(
            NetworkErrorContext{"TCP connection failed"_el, "No resolved IP endpoint accepted the connection."_el});
        if (context.reason() == NetworkErrorReason::Unknown) {
            context.setReason(NetworkErrorReason::SocketOperationFailed);
        }
        if (_requestedEndpoint.has_value()) {
            context.setRemoteEndpoint(*_requestedEndpoint);
        }
        finishFailed(std::move(context));
        return;
    }
    const auto endpoint = _resolvedEndpoints.getRefOrThrow(unit::ItemIndex::fromSizeT(_nextEndpoint++));
    cleanupDevice(true);
    try {
        auto device = _createDevice(_driver, _bufferLimits.receive(), createCallbacks(generation));
        {
            const auto lock = std::scoped_lock{_dataMutex};
            _device = std::move(device);
        }
        const auto lock = std::scoped_lock{_dataMutex};
        _device->connect(endpoint);
    } catch (const NetworkError &error) {
        _lastConnectError = error.context();
        startNextEndpoint(generation);
    } catch (const RuntimeError &error) {
        _lastConnectError =
            NetworkErrorContext{"TCP connection failed"_el, error.reason()}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setRemoteEndpoint(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
        startNextEndpoint(generation);
    }
}

void TcpConnection::deliverConnected(
    const std::uint64_t generation, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || !isStarting()) {
        return;
    }
    {
        const auto lock = std::scoped_lock{_dataMutex};
        _localEndpoint = std::move(localEndpoint);
        _remoteEndpoint = std::move(remoteEndpoint);
    }
    _state.store(ConnectionState::Active);
    updateReceiving();
    const auto callback = _onConnected;
    if (callback) {
        callback();
    }
}

void TcpConnection::deliverError(const std::uint64_t generation, NetworkErrorContext context) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation)) {
        return;
    }
    if (isStarting() && _nextEndpoint < _resolvedEndpoints.count().toSizeT()) {
        _lastConnectError = std::move(context);
        startNextEndpoint(generation);
        return;
    }
    if (isStarting() && !_resolvedEndpoints.isEmpty()) {
        _lastConnectError = std::move(context);
        startNextEndpoint(generation);
        return;
    }
    finishFailed(std::move(context));
}

void TcpConnection::deliverTimeout(const std::uint64_t generation) {
    const auto lifecycleLock = std::scoped_lock{_lifecycleMutex};
    if (!isCurrent(generation) || !isStarting()) {
        return;
    }
    auto context = NetworkErrorContext{
        "TCP connection timed out"_el, "The remote endpoint could not be reached within the configured timeout."_el};
    context.setReason(NetworkErrorReason::Timeout);
    if (_requestedEndpoint.has_value()) {
        context.setRemoteEndpoint(*_requestedEndpoint);
    }
    finishFailed(std::move(context));
}

void TcpConnection::validateLimits(const SocketBufferLimits &limits) const {
    if (!limits.send().isFinite() || limits.send().isZero()) {
        throw ParameterFailure{"The TCP send queue limit must be positive and finite."_el, "bufferLimits.send"_el};
    }
    if (!limits.receive().isFinite() || limits.receive().isZero()) {
        throw ParameterFailure{
            "The TCP receive queue limit must be positive and finite."_el, "bufferLimits.receive"_el};
    }
}

auto TcpConnection::isCurrent(const std::uint64_t generation) const noexcept -> bool {
    return _generation.load() == generation;
}

auto TcpConnection::isStarting() const noexcept -> bool {
    const auto current = _state.load();
    return current == ConnectionState::Connecting || current == ConnectionState::Accepting;
}

}
