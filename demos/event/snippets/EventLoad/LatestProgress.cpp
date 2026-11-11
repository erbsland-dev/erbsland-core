// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LatestProgress.hpp"

#include <erbsland/EventTarget.hpp>

#include <utility>

namespace demo {

LatestProgress::LatestProgress(el::EventTargetPtr target, std::function<void(int)> display) :
    _target{std::move(target)}, _display{std::move(display)} {
}

void LatestProgress::publish(const int progress) {
    auto lock = std::scoped_lock{_mutex};
    _latest = progress;
    if (_pending) {
        return;
    }
    // invoke() queues delivery; the handler never runs inline under this lock.
    _target->invoke([this]() -> void { deliver(); });
    _pending = true;
}

void LatestProgress::deliver() {
    auto progress = int{};
    {
        auto lock = std::scoped_lock{_mutex};
        progress = _latest;
        _pending = false;
    }
    // A producer can now schedule another notification, even while display is running.
    _display(progress);
}

}
