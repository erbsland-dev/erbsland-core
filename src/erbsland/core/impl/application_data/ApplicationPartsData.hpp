// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationEventData_fwd.hpp"
#include "ApplicationLifecycleData_fwd.hpp"
#include "ApplicationPartsData_fwd.hpp"

#include "../../../event/EventSubscription.hpp"
#include "../../ApplicationPartManager_fwd.hpp"

#include <memory>
#include <mutex>

namespace erbsland::core::impl {

/// Application-integrated part manager and its lifecycle binding.
/// @tested{ApplicationPartApplicationTest ApplicationServiceLifecycleTest}
class ApplicationPartsData final {
public:
    /// Access the part manager, creating and binding it on first use.
    [[nodiscard]] auto manager(
        const ApplicationEventDataPtr &eventData, const std::shared_ptr<ApplicationLifecycleData> &lifecycleData)
        -> ApplicationPartManagerPtr;
    /// Access the part manager without creating it.
    [[nodiscard]] auto managerIfExists() -> ApplicationPartManagerPtr;
    /// Bring a created part manager to a terminal state.
    void stop(ApplicationEventData &eventData);

private:
    std::mutex _mutex;                           ///< Serializes manager creation and access.
    ApplicationPartManagerPtr _manager;          ///< Application-integrated part manager.
    event::EventSubscription _stateSubscription; ///< Binding to application lifecycle state.
};

}
