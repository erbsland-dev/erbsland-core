// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartCallback.hpp"
#include "ApplicationPartManagerEventEditor_fwd.hpp"

#include "../event/EventSubscription.hpp"

namespace erbsland::core {

/// Editor for application-part manager lifecycle observers.
/// The manager owns this stable editor. Each added callback remains active while its returned subscription is retained.
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest ApplicationServiceLifecycleTest}
class ApplicationPartManagerEventEditor {
public:
    // defaults
    virtual ~ApplicationPartManagerEventEditor() = default;

public:
    /// Add a manager-state observer.
    /// @param callback The callback invoked for later manager-state transitions.
    /// @return A subscription retaining the observer.
    [[nodiscard]] virtual auto addStateChanged(ApplicationPartManagerStateChangedFn callback)
        -> event::EventSubscription = 0;
    /// Add a part-state observer.
    /// @param callback The callback invoked for later part-state transitions.
    /// @return A subscription retaining the observer.
    [[nodiscard]] virtual auto addPartStateChanged(ApplicationPartStateChangedFn callback)
        -> event::EventSubscription = 0;
};

}
