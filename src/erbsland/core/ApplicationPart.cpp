// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPart.hpp"

#include "ApplicationPartManagerAccess.hpp"

#include "../err/LogicError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::core {

using namespace text::literals;

auto ApplicationPart::partManager() const -> ApplicationPartManagerAccess & {
    const auto manager = _manager.lock();
    if (manager == nullptr) {
        throw err::LogicError{"The application part is not bound to a manager."_el};
    }
    return *manager;
}

auto ApplicationPart::events() const -> event::EventsPtr {
    if (_events == nullptr) {
        throw err::LogicError{"The application part has no event target before startup."_el};
    }
    return _events;
}

auto ApplicationPart::automaticStart() -> bool {
    return true;
}

auto ApplicationPart::shutdownTimeout() const noexcept -> time::TimeDelta {
    return time::TimeDelta::seconds(30);
}

void ApplicationPart::initialize() {
}

void ApplicationPart::running() {
}

void ApplicationPart::stopping() {
    completeStopping();
}

void ApplicationPart::cleanup() noexcept {
}

void ApplicationPart::completeStopping() noexcept {
    auto callback = std::function<void()>{};
    {
        const auto lock = std::scoped_lock{_stoppingMutex};
        callback = _completeStoppingFn;
    }
    if (callback) {
        callback();
    }
}

void ApplicationPart::registerCommandLineOptions([[maybe_unused]] const options::OptionsPtr &options) {
}

void ApplicationPart::parseCommandLine([[maybe_unused]] const options::OptionValuesPtr &values) {
}

void ApplicationPart::bind(ApplicationPartManagerAccessWeakPtr manager, event::EventsPtr events) {
    _manager = std::move(manager);
    _events = std::move(events);
}

void ApplicationPart::setState(const ApplicationPartState state) noexcept {
    _state = state;
}

}
