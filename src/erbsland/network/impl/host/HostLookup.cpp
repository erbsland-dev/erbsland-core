// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostLookup.hpp"

#include "HostLookupEventEditor.hpp"
#include "HostResolverErrorContext.hpp"

#include "../ResolverService.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../event/Events.hpp"
#include "../../../system/PlatformError.hpp"
#include "../../../system/PlatformErrorContext.hpp"
#include "../../../text/Literals.hpp"

#include <exception>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HostLookup::HostLookup(event::EventsPtr ownerEvents, HostResolverPtr resolver) :
    network::HostLookup{std::move(ownerEvents)}, _resolver{std::move(resolver)} {
}

HostLookup::~HostLookup() {
    cancel();
}

auto HostLookup::host() const -> std::optional<Host> {
    const auto lock = std::scoped_lock{_operationMutex};
    return _host;
}

auto HostLookup::state() const noexcept -> NetworkSourceState {
    return _state.load();
}

void HostLookup::start(Host host, HostLookupOptions options) {
    verifyCurrentOwnerEvents();
    if (!options.timeout().isPositive()) {
        throw err::ParameterError{"The host lookup timeout must be positive."_el, "options.timeout"_el};
    }
    if (options.maximumAttempts().isZero() || options.maximumAttempts().isInfinite()) {
        throw err::ParameterError{
            "The maximum host lookup attempt count must be finite and positive."_el, "options.maximumAttempts"_el};
    }
    if (options.retryDelay().isNegative()) {
        throw err::ParameterError{"The host lookup retry delay must not be negative."_el, "options.retryDelay"_el};
    }

    auto operation = HostLookupOperationPtr{};
    {
        const auto lock = std::scoped_lock{_operationMutex};
        if (_operation != nullptr) {
            throw err::LogicError{"A host lookup operation is already active."_el};
        }
        operation = std::make_shared<HostLookupOperation>(std::move(host), options);
        _host = operation->host;
        _operation = operation;
        _state.store(NetworkSourceState::Starting);
    }

    try {
        const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
        ownerEvents()->invokeAfter(options.timeout(), [weakSelf, operation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->deliverTimeout(operation);
            }
        });
        if (const auto address = operation->host.address(); address.has_value()) {
            startNumeric(operation, *address);
            return;
        }
        startNamed(operation, *operation->host.name());
    } catch (...) {
        operation->completionClaimed.store(true);
        finishOperation(operation);
        throw;
    }
}

void HostLookup::cancel() noexcept {
    auto operation = HostLookupOperationPtr{};
    {
        const auto lock = std::scoped_lock{_operationMutex};
        operation = _operation;
        if (operation == nullptr || operation->completionClaimed.exchange(true)) {
            return;
        }
        _host.reset();
        _state.store(NetworkSourceState::Closed);
    }
    try {
        const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
        ownerEvents()->invoke([weakSelf, operation]() -> void {
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->deliverCancelled(operation);
            }
        });
    } catch (...) { // NOLINT(*-empty-catch)
        finishOperation(operation);
    }
}

auto HostLookup::events() -> network::HostLookupEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<impl::HostLookupEventEditor>(shared_from_this(), std::move(target));
    }
    return *_eventEditor;
}

void HostLookup::startNumeric(const HostLookupOperationPtr &operation, IpAddress address) {
    postResolved(operation, util::List<IpAddress>{std::move(address)});
}

void HostLookup::startNamed(const HostLookupOperationPtr &operation, HostName hostName) {
    submitNamedAttempt(operation, hostName);
}

void HostLookup::submitNamedAttempt(const HostLookupOperationPtr &operation, const HostName &hostName) {
    const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
    const auto resolver = _resolver;
    operation->attemptCount.fetch_add(1U);
    ResolverService::submit([weakSelf, operation, resolver, hostName]() -> void {
        if (operation->completionClaimed.load()) {
            return;
        }
        auto addresses = util::List<IpAddress>{};
        try {
            addresses = resolver->resolve(hostName);
        } catch (const system::PlatformError &error) {
            if (isRetryable(error) && operation->attemptCount.load() < operation->options.maximumAttempts().toSizeT() &&
                time::TimePoint::now() < operation->deadline) {
                if (const auto self = weakSelf.lock(); self != nullptr) {
                    self->scheduleNamedRetry(operation, hostName);
                }
                return;
            }
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->postError(
                    operation,
                    errorReason(error),
                    "The host name could not be resolved to an IP address."_el,
                    error.context());
            }
            return;
        }
        if (operation->completionClaimed.load()) {
            return;
        }
        addresses = normalizedAddresses(addresses);
        if (const auto self = weakSelf.lock(); self != nullptr) {
            if (addresses.isEmpty()) {
                self->postError(
                    operation, NetworkErrorReason::NoAddresses, "The resolver returned no supported IP addresses."_el);
                return;
            }
            self->postResolved(operation, std::move(addresses));
        }
    });
}

void HostLookup::scheduleNamedRetry(const HostLookupOperationPtr &operation, HostName hostName) {
    const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
    ownerEvents()->invokeAfter(
        operation->options.retryDelay(), [weakSelf, operation, hostName = std::move(hostName)]() {
            if (operation->completionClaimed.load() || time::TimePoint::now() >= operation->deadline) {
                return;
            }
            if (const auto self = weakSelf.lock(); self != nullptr) {
                self->submitNamedAttempt(operation, hostName);
            }
        });
}

void HostLookup::postResolved(const HostLookupOperationPtr &operation, util::List<IpAddress> addresses) {
    const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, operation, addresses = std::move(addresses)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverResolved(operation, std::move(addresses));
        }
    });
}

void HostLookup::postError(
    const HostLookupOperationPtr &operation,
    const NetworkErrorReason reason,
    text::String description,
    system::PlatformErrorContextConstPtr platformContext) {
    auto context = NetworkErrorContext{"Host lookup failed"_el, std::move(description)};
    context.setReason(reason).setHost(operation->host);
    if (platformContext != nullptr) {
        context.setPlatformContext(std::move(platformContext));
    }
    const auto weakSelf = std::weak_ptr<HostLookup>{std::static_pointer_cast<HostLookup>(shared_from_this())};
    ownerEvents()->invoke([weakSelf, operation, context = std::move(context)]() mutable -> void {
        if (const auto self = weakSelf.lock(); self != nullptr) {
            self->deliverError(operation, std::move(context));
        }
    });
}

void HostLookup::deliverResolved(const HostLookupOperationPtr &operation, util::List<IpAddress> addresses) {
    if (time::TimePoint::now() >= operation->deadline) {
        deliverTimeout(operation);
        return;
    }
    if (!claimCompletion(operation, NetworkSourceState::Closed)) {
        return;
    }
    try {
        const auto callback = _onResolved;
        if (callback) {
            callback(addresses);
        }
    } catch (...) {
        finishOperation(operation);
        throw;
    }
    finishOperationAndNotify(operation);
}

void HostLookup::deliverError(const HostLookupOperationPtr &operation, NetworkErrorContext context) {
    if (context.reason() != NetworkErrorReason::Timeout && time::TimePoint::now() >= operation->deadline) {
        deliverTimeout(operation);
        return;
    }
    if (!claimCompletion(operation, NetworkSourceState::Failed)) {
        return;
    }
    try {
        const auto callback = _onError;
        if (callback) {
            callback(context);
        }
    } catch (...) {
        finishOperation(operation);
        throw;
    }
    finishOperationAndNotify(operation);
}

void HostLookup::deliverTimeout(const HostLookupOperationPtr &operation) {
    auto context = NetworkErrorContext{
        "Host lookup timed out"_el, "The host could not be resolved within the configured timeout."_el};
    context.setReason(NetworkErrorReason::Timeout).setHost(operation->host);
    deliverError(operation, std::move(context));
}

void HostLookup::deliverCancelled(const HostLookupOperationPtr &operation) {
    finishOperationAndNotify(operation);
}

auto HostLookup::claimCompletion(const HostLookupOperationPtr &operation, const NetworkSourceState finalState) noexcept
    -> bool {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_operation != operation || operation->completionClaimed.exchange(true)) {
        return false;
    }
    _state.store(finalState);
    return true;
}

auto HostLookup::finishOperation(const HostLookupOperationPtr &operation) noexcept -> bool {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_operation != operation) {
        return false;
    }
    _operation.reset();
    _host.reset();
    _state.store(NetworkSourceState::Inactive);
    return true;
}

void HostLookup::finishOperationAndNotify(const HostLookupOperationPtr &operation) {
    if (!finishOperation(operation)) {
        return;
    }
    const auto callback = _onFinal;
    if (callback) {
        callback();
    }
}

auto HostLookup::isRetryable(const system::PlatformError &error) noexcept -> bool {
    const auto context = std::dynamic_pointer_cast<const HostResolverErrorContext>(error.context());
    return context != nullptr && context->isRetryable();
}

auto HostLookup::errorReason(const system::PlatformError &error) noexcept -> NetworkErrorReason {
    if (error.context() != nullptr && error.context()->category() == system::PlatformErrorCategory::NotFound) {
        return NetworkErrorReason::HostNotFound;
    }
    return NetworkErrorReason::HostResolutionFailed;
}

auto HostLookup::normalizedAddresses(const util::List<IpAddress> &addresses) -> util::List<IpAddress> {
    auto result = util::List<IpAddress>{};
    for (const auto &address : addresses) {
        if (!result.contains(address)) {
            result.append(address);
        }
    }
    return result;
}

}
