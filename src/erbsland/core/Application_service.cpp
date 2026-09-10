// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "impl/application_data/ApplicationLifecycleData.hpp"
#include "impl/application_data/ApplicationRuntimeData.hpp"
#include "impl/ApplicationData.hpp"

#include "../system/impl/service/ServiceLifecycle.hpp"
#include "../time/TimeDelta.hpp"

#include <memory>

namespace erbsland::core {

void Application::enableServiceLifecycle() {
    const auto weakData = std::weak_ptr<impl::ApplicationData>{_data};
    const auto applicationRunStarted = _data->runtime().get()->isRunStarted();
    _data->lifecycle().get()->enableService(applicationRunStarted, [weakData]() {
        return system::impl::ServiceLifecycle::create([weakData]() -> void {
            if (const auto data = weakData.lock(); data != nullptr) {
                requestQuit(data, unit::ExitCode::success());
            }
        });
    });
}

void Application::reportStartupPending(const time::TimeDelta expectedRemainingTime) {
    _data->lifecycle().get()->reportStartupPending(expectedRemainingTime);
}

void Application::reportStartupComplete() {
    _data->lifecycle().get()->reportStartupComplete();
}

}
