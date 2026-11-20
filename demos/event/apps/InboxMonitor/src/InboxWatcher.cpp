// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InboxWatcher.hpp"

#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/PathWalker.hpp>

namespace demo {

using namespace el::text::literals;

InboxWatcher::InboxWatcher(el::Path directory) : EventSource{el::currentLoop()}, _directory{std::move(directory)} {
}

auto InboxWatcher::create(el::Path directory) -> std::shared_ptr<InboxWatcher> {
    return std::shared_ptr<InboxWatcher>{new InboxWatcher{std::move(directory)}};
}

auto InboxWatcher::events() -> InboxEvents & {
    verifyCurrentOwnerLoop();
    return _events;
}

void InboxWatcher::start() {
    verifyCurrentOwnerLoop();
    if (_timer) {
        throw el::LogicFailure{"An inbox watch can only start once."_el};
    }
    const auto self = std::static_pointer_cast<InboxWatcher>(shared_from_this());
    _timer = ownerLoop()->createTimer([weak = std::weak_ptr{self}]() -> void {
        if (const auto watcher = weak.lock()) {
            watcher->scan();
        }
    });
    _timer->startFixedDelay(el::Seconds{1});
    self->scan();
}

void InboxWatcher::stop() {
    verifyCurrentOwnerLoop();
    if (_timer) {
        _timer->stop();
    }
}

void InboxWatcher::scan() {
    auto visible = el::StringSet{};
    auto arrivals = el::PathList{};
    auto options = el::PathWalkOptions{};
    options.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip);
    options.setDescendFilter([this](const el::Path &path, el::PathType) -> bool { return path == _directory; });
    _directory.walker().walkOrThrow(
        [&](const el::Path &path) -> el::PathWalkStatus {
            visible.insert(path.name());
            if (!_known.contains(path.name())) {
                arrivals.append(path);
            }
            return el::PathWalkStatus::Continue;
        },
        options);
    _known = std::move(visible);
    for (const auto &path : arrivals) {
        if (!_timer->isActive()) {
            break;
        }
        if (const auto handler = _events._arrival) {
            (*handler)(path);
        }
    }
}

}
