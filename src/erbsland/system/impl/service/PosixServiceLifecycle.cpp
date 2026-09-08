// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixServiceLifecycle.hpp"

#include "../signal/ProcessSignalDispatcher.hpp"

#include <utility>

namespace erbsland::system::impl {

auto ServiceLifecycle::create(StopFn stopFn) -> ServiceLifecyclePtr {
    return std::make_unique<PosixServiceLifecycle>(std::move(stopFn));
}

PosixServiceLifecycle::PosixServiceLifecycle(StopFn stopFn) : _stopFn{std::move(stopFn)} {
}

auto PosixServiceLifecycle::run(RunFn runFn) -> int {
    const auto signalSubscription =
        ProcessSignalDispatcher::instance().addSignal([this](const ProcessSignal signal, bool &claimed) -> void {
            if (signal != ProcessSignal::Interrupt && signal != ProcessSignal::Terminate) {
                return;
            }
            claimed = true;
            _stopFn();
        });
    return runFn();
}

void PosixServiceLifecycle::reportStartupPending([[maybe_unused]] const time::TimeDelta expectedRemainingTime) {
}

void PosixServiceLifecycle::reportStartupComplete() {
}

void PosixServiceLifecycle::reportStopping() noexcept {
}

}
