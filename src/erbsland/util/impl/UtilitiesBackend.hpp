// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UtilitiesBackend_fwd.hpp"

#include "../Utilities.hpp"

#include "../../event/EventBackend.hpp"
#include "../../event/EventBackendTarget.hpp"

namespace erbsland::util::impl {

/// Lazy built-in factory for independently owned utility event sources.
/// @tested{UtilitiesTest}
class UtilitiesBackend final : public event::EventBackend, public Utilities {
public: // implement EventBackend
    auto backendId() const noexcept -> event::EventBackendId override;
    void attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr driver) override;
    void poll(time::TimePoint now) override;
    auto handleEvent(const event::Event &event) -> bool override;
    auto nextWakeTime() const -> std::optional<time::TimePoint> override;

public: // implement Utilities
    auto createWorkerPool(const WorkerPoolOptions &options) -> WorkerPoolPtr override;

private:
    event::EventBackendTargetWeakPtr _target; ///< Attached loop, without retaining it or its sources.
};

}
