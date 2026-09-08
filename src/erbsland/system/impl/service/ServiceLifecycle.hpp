// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ServiceLifecycle_fwd.hpp"

#include "../../../time/TimeDelta.hpp"

#include <functional>

namespace erbsland::system::impl {

/// Platform lifecycle bridge for a foreground daemon or Windows service.
/// @notest{Abstract platform interface; concrete backends own the tests.}
class ServiceLifecycle {
public:
    using RunFn = std::function<int()>;
    using StopFn = std::function<void()>;

public:
    // defaults
    virtual ~ServiceLifecycle() = default;

public:
    /// Execute the application through the platform lifecycle.
    [[nodiscard]] virtual auto run(RunFn runFn) -> int = 0;
    /// Report startup progress with an expected remaining duration.
    virtual void reportStartupPending(time::TimeDelta expectedRemainingTime) = 0;
    /// Report that application startup completed.
    virtual void reportStartupComplete() = 0;
    /// Report that application shutdown started.
    virtual void reportStopping() noexcept = 0;

public:
    /// Create the platform lifecycle bridge.
    [[nodiscard]] static auto create(StopFn stopFn) -> ServiceLifecyclePtr;
};

}
