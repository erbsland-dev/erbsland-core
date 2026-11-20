// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InboxEvents.hpp"

#include "InboxWatcher.hpp"

namespace demo {

auto InboxEvents::source() const -> el::EventSourcePtr {
    return _watcher.shared_from_this();
}

auto InboxEvents::target() const noexcept -> el::EventTargetPtr {
    return _watcher.ownerLoop();
}

auto InboxEvents::onFileArrived(Arrival handler) -> InboxEvents & {
    _watcher.verifyCurrentOwnerLoop();
    _arrival.reset();
    if (handler) {
        _arrival = std::make_shared<Arrival>(std::move(handler));
    }
    return *this;
}

}
