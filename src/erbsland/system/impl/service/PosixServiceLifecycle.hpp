// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ServiceLifecycle.hpp"

namespace erbsland::system::impl {

/// Foreground POSIX lifecycle with graceful termination-signal handling.
/// @tested{PosixApplicationServiceLifecycleTest}
class PosixServiceLifecycle final : public ServiceLifecycle {
public:
    /// Create a POSIX foreground lifecycle.
    /// @param stopFn Callback that requests graceful application shutdown.
    explicit PosixServiceLifecycle(StopFn stopFn);

    // defaults/deletions
    ~PosixServiceLifecycle() override = default;

public: // implement ServiceLifecycle
    [[nodiscard]] auto run(RunFn runFn) -> int override;
    void reportStartupPending(time::TimeDelta expectedRemainingTime) override;
    void reportStartupComplete() override;
    void reportStopping() noexcept override;

private:
    StopFn _stopFn; ///< Graceful application-stop callback.
};

}
