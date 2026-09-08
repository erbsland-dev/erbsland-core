// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartsData.hpp"

#include "ApplicationEventData.hpp"
#include "ApplicationLifecycleData.hpp"

#include "../../../event/impl/CurrentEventsScope.hpp"
#include "../../ApplicationPartManager.hpp"
#include "../../ApplicationPartManagerState.hpp"

#include <exception>

namespace erbsland::core::impl {

auto ApplicationPartsData::manager(
    const ApplicationEventDataPtr &eventData, const std::shared_ptr<ApplicationLifecycleData> &lifecycleData)
    -> ApplicationPartManagerPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (_manager == nullptr) {
        _manager = core::ApplicationPartManager::create(eventData->events());
        const auto weakEventData = std::weak_ptr<ApplicationEventData>{eventData};
        const auto weakLifecycleData = std::weak_ptr<ApplicationLifecycleData>{lifecycleData};
        _stateSubscription =
            _manager->events().addStateChanged([weakEventData, weakLifecycleData](const auto state) -> void {
                const auto currentLifecycleData = weakLifecycleData.lock();
                if (currentLifecycleData != nullptr) {
                    if (state == ApplicationPartManagerState::Running) {
                        currentLifecycleData->reportAutomaticStartupComplete();
                    }
                    if (state == ApplicationPartManagerState::Stopping) {
                        currentLifecycleData->reportServiceStopping();
                    }
                }
                if (state == ApplicationPartManagerState::Stopped || state == ApplicationPartManagerState::Failed) {
                    if (const auto currentEventData = weakEventData.lock(); currentEventData != nullptr) {
                        currentEventData->quit();
                    }
                }
            });
    }
    return _manager;
}

auto ApplicationPartsData::managerIfExists() -> ApplicationPartManagerPtr {
    const auto lock = std::scoped_lock{_mutex};
    return _manager;
}

void ApplicationPartsData::stop(ApplicationEventData &eventData) {
    const auto manager = managerIfExists();
    if (manager == nullptr) {
        return;
    }
    auto managerState = manager->state();
    if (managerState == ApplicationPartManagerState::Ready || managerState == ApplicationPartManagerState::Starting ||
        managerState == ApplicationPartManagerState::Running) {
        manager->stop();
        managerState = manager->state();
    }
    if (managerState == ApplicationPartManagerState::Ready || managerState == ApplicationPartManagerState::Starting ||
        managerState == ApplicationPartManagerState::Running || managerState == ApplicationPartManagerState::Stopping) {
        auto currentEventsScope = event::impl::CurrentEventsScope{eventData.events()};
        eventData.eventLoop().run();
    }
    if (manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
}

}
