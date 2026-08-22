// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookup_fwd.hpp"
#include "HostLookupEventEditor.hpp"
#include "HostLookupOperation.hpp"
#include "HostResolver.hpp"

#include "../../../system/PlatformError.hpp"
#include "../../host_lookup/HostLookup.hpp"
#include "../../source/NetworkErrorContext.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace erbsland::network::impl {

/// Native asynchronous host lookup implementation.
/// @tested{HostLookupTest HostLookupLiveDnsTest}
class HostLookup final : public network::HostLookup {
    friend class HostLookupEventEditor;

public:
    /// Create an inactive lookup.
    /// @param ownerEvents The owner event loop.
    /// @param resolver The native resolver.
    HostLookup(event::EventsPtr ownerEvents, HostResolverPtr resolver);
    ~HostLookup() override;

public: // implement network::HostLookup
    [[nodiscard]] auto host() const -> std::optional<Host> override;
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    void start(Host host, HostLookupOptions options) override;
    void cancel() noexcept override;
    [[nodiscard]] auto events() -> network::HostLookupEventEditor & override;

private:
    /// Resolve a numeric address without submitting a DNS request.
    void startNumeric(const HostLookupOperationPtr &operation, IpAddress address);
    /// Begin resolving a host name.
    void startNamed(const HostLookupOperationPtr &operation, HostName hostName);
    /// Submit one asynchronous resolution attempt for a host name.
    void submitNamedAttempt(const HostLookupOperationPtr &operation, const HostName &hostName);
    /// Schedule the next resolution attempt for a host name.
    void scheduleNamedRetry(const HostLookupOperationPtr &operation, HostName hostName);
    /// Post a successful resolution result to the owner event loop.
    void postResolved(const HostLookupOperationPtr &operation, util::List<IpAddress> addresses);
    /// Post a resolution error to the owner event loop.
    void postError(
        const HostLookupOperationPtr &operation,
        NetworkErrorReason reason,
        text::String description,
        system::PlatformErrorContextConstPtr platformContext = {});
    /// Deliver resolved addresses on the owner event loop.
    void deliverResolved(const HostLookupOperationPtr &operation, util::List<IpAddress> addresses);
    /// Deliver a resolution error on the owner event loop.
    void deliverError(const HostLookupOperationPtr &operation, NetworkErrorContext context);
    /// Deliver a timeout error on the owner event loop.
    void deliverTimeout(const HostLookupOperationPtr &operation);
    /// Deliver cancellation on the owner event loop.
    void deliverCancelled(const HostLookupOperationPtr &operation);
    /// Atomically claim completion for an operation.
    [[nodiscard]] auto claimCompletion(const HostLookupOperationPtr &operation, NetworkSourceState finalState) noexcept
        -> bool;
    /// Remove a completed operation from the active state.
    auto finishOperation(const HostLookupOperationPtr &operation) noexcept -> bool;
    /// Remove a completed operation and notify its final callback.
    void finishOperationAndNotify(const HostLookupOperationPtr &operation);
    /// Test if a platform resolver error permits another attempt.
    [[nodiscard]] static auto isRetryable(const system::PlatformError &error) noexcept -> bool;
    /// Map a platform resolver error to its public error reason.
    [[nodiscard]] static auto errorReason(const system::PlatformError &error) noexcept -> NetworkErrorReason;
    /// Remove duplicate and unsupported addresses from a resolution result.
    [[nodiscard]] static auto normalizedAddresses(const util::List<IpAddress> &addresses) -> util::List<IpAddress>;

private:
    HostResolverPtr _resolver;                                            ///< Native resolver.
    std::unique_ptr<HostLookupEventEditor> _eventEditor;                  ///< Lazily created source-owned editor.
    HostResolvedFn _onResolved;                                           ///< Successful resolution handler.
    NetworkErrorFn _onError;                                              ///< Resolver error handler.
    NetworkEventFn _onFinal;                                              ///< Final operation handler.
    mutable std::mutex _operationMutex;                                   ///< Protects current operation data.
    HostLookupOperationPtr _operation;                                    ///< Current operation, if any.
    std::optional<Host> _host;                                            ///< Host of the current operation.
    std::atomic<NetworkSourceState> _state{NetworkSourceState::Inactive}; ///< Public lifecycle state.
};

}
