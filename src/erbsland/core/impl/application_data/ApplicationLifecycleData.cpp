// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationLifecycleData.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"

#include <utility>

namespace erbsland::core::impl {

using namespace err;
using namespace text::literals;

void ApplicationLifecycleData::enableService(const bool applicationRunStarted, ServiceLifecycleFactory factory) {
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle != nullptr) {
        return;
    }
    if (applicationRunStarted) {
        throw LogicFailure{"Service lifecycle support must be enabled before running the application."_el};
    }
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    if (_configuredServiceLifecycle != nullptr) {
        _serviceLifecycle = std::move(_configuredServiceLifecycle);
        return;
    }
#endif
    _serviceLifecycle = factory();
}

#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
void ApplicationLifecycleData::setServiceLifecycle(system::impl::ServiceLifecyclePtr serviceLifecycle) {
    _configuredServiceLifecycle = std::move(serviceLifecycle);
}

#endif

auto ApplicationLifecycleData::run(system::impl::ServiceLifecycle::RunFn runFn) -> int {
    auto *serviceLifecycle = static_cast<system::impl::ServiceLifecycle *>(nullptr);
    {
        const auto lock = std::scoped_lock{_mutex};
        serviceLifecycle = _serviceLifecycle.get();
    }
    if (serviceLifecycle != nullptr) {
        return serviceLifecycle->run(std::move(runFn));
    }
    return runFn();
}

void ApplicationLifecycleData::reportStartupPending(const time::TimeDelta expectedRemainingTime) {
    if (!expectedRemainingTime.isPositive()) {
        throw ParameterFailure{"The expected remaining startup time must be positive."_el, "expectedRemainingTime"_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle == nullptr) {
        throw LogicFailure{"Service lifecycle support must be enabled before reporting startup state."_el};
    }
    if (_startupComplete) {
        throw LogicFailure{"Application startup was already reported as complete."_el};
    }
    _startupExplicitlyPending = true;
    _serviceLifecycle->reportStartupPending(expectedRemainingTime);
}

void ApplicationLifecycleData::reportStartupComplete() {
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle == nullptr) {
        throw LogicFailure{"Service lifecycle support must be enabled before reporting startup state."_el};
    }
    if (_startupComplete) {
        return;
    }
    if (isShutdownRequested()) {
        throw LogicFailure{"Application shutdown started before startup completed."_el};
    }
    _startupComplete = true;
    _serviceLifecycle->reportStartupComplete();
}

void ApplicationLifecycleData::reportAutomaticStartupPending() {
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle == nullptr || _startupComplete || _startupExplicitlyPending || isShutdownRequested()) {
        return;
    }
    _serviceLifecycle->reportStartupPending(time::TimeDelta::seconds(30));
}

void ApplicationLifecycleData::reportAutomaticStartupComplete() {
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle == nullptr || _startupComplete || _startupExplicitlyPending || isShutdownRequested()) {
        return;
    }
    _startupComplete = true;
    _serviceLifecycle->reportStartupComplete();
}

void ApplicationLifecycleData::reportServiceStopping() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    if (_serviceLifecycle != nullptr && !_serviceStoppingReported) {
        _serviceStoppingReported = true;
        _serviceLifecycle->reportStopping();
    }
}

void ApplicationLifecycleData::requestShutdown() noexcept {
    _shutdownRequested.store(true, std::memory_order_release);
}

auto ApplicationLifecycleData::isShutdownRequested() const noexcept -> bool {
    return _shutdownRequested.load(std::memory_order_acquire);
}

}
