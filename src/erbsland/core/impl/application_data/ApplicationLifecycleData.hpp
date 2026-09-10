// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationLifecycleData_fwd.hpp"

#include "../../../system/impl/service/ServiceLifecycle.hpp"

#include <atomic>
#include <functional>
#include <mutex>

namespace erbsland::core::impl {

/// Shared and synchronized state for the application lifecycle.
/// @tested{ApplicationServiceLifecycleTest}
class ApplicationLifecycleData final {
public:
    /// Factory for the platform-specific service lifecycle.
    using ServiceLifecycleFactory = std::function<system::impl::ServiceLifecyclePtr()>;

public:
    /// Enable native service integration using a lazily invoked factory.
    void enableService(bool applicationRunStarted, ServiceLifecycleFactory factory);
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    /// Configure the service lifecycle implementation used when service integration is enabled.
    /// This method must only be called during application construction, before service integration is enabled.
    void setServiceLifecycle(system::impl::ServiceLifecyclePtr serviceLifecycle);
#endif
    /// Run application work through native service integration when enabled.
    [[nodiscard]] auto run(system::impl::ServiceLifecycle::RunFn runFn) -> int;
    /// Report explicit startup progress.
    void reportStartupPending(time::TimeDelta expectedRemainingTime);
    /// Report explicit startup completion.
    void reportStartupComplete();
    /// Report framework-managed startup progress unless explicit progress takes precedence.
    void reportAutomaticStartupPending();
    /// Report framework-managed startup completion unless explicit progress takes precedence.
    void reportAutomaticStartupComplete();
    /// Report graceful stopping to native service integration once.
    void reportServiceStopping() noexcept;
    /// Record that graceful shutdown was requested.
    void requestShutdown() noexcept;
    /// Test whether graceful shutdown was requested.
    [[nodiscard]] auto isShutdownRequested() const noexcept -> bool;

private:
    mutable std::mutex _mutex;                                     ///< Protects all non-atomic lifecycle state.
    system::impl::ServiceLifecyclePtr _serviceLifecycle;           ///< Optional native lifecycle integration.
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    system::impl::ServiceLifecyclePtr _configuredServiceLifecycle; ///< Pending injected lifecycle implementation.
#endif
    std::atomic<bool> _shutdownRequested{false};                   ///< Prevents work from starting after shutdown.
    bool _startupExplicitlyPending{false};                         ///< User disabled automatic startup completion.
    bool _startupComplete{false};                                  ///< Readiness was reported once.
    bool _serviceStoppingReported{false};                          ///< Stopping was forwarded to the platform once.
};

}
