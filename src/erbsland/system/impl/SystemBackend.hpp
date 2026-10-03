// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../System.hpp"

#include "../../event/EventBackend.hpp"
#include "../../event/EventBackendTarget.hpp"

#include <vector>

namespace erbsland::system::impl {

/// Lazy built-in frontend and coalesced native-work dispatcher.
/// @tested{SubprocessEventTest}
class SystemBackend final : public event::EventBackend, public System {
public: // implement EventBackend
    auto backendId() const noexcept -> event::EventBackendId override;
    void attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr driver) override;
    void poll(time::TimePoint now) override;
    auto handleEvent(const event::Event &event) -> bool override;
    auto nextWakeTime() const -> std::optional<time::TimePoint> override;

public: // implement System
    auto createSubprocess(
        const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
        -> SubprocessPtr override;

private:
    void bindSubprocess(const SubprocessPtr &process) override;

private:
    event::EventBackendTargetWeakPtr _target;        ///< Attached loop target.
    std::vector<std::weak_ptr<Subprocess>> _sources; ///< Observed sources without extending ownership.
};

}
