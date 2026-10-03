// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSourceDemos.hpp"
#include "LifecycleObserver.hpp"

#include <erbsland/ApplicationPartManager.hpp>
#include <erbsland/ApplicationPartManagerState.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/StandardStreams.hpp>

#include <exception>
#include <utility>

namespace demo {

/// Retain independent observers, transfer a subscription, and cancel before the next notification.
/// Cancelling a subscription changes observation, not the publisher's lifecycle.
void observerLifetime() {
    const auto loop = el::EventLoop::create();
    auto manager = el::ApplicationPartManager::create(loop);
    auto observedCount = 0;
    auto diagnostic = manager->events().addStateChanged([](const el::ApplicationPartManagerState) -> void {
        el::io::printLine("Diagnostic observer received a transition."_el);
    });
    auto subscription = manager->events().addStateChanged(
        [&observedCount](const el::ApplicationPartManagerState) -> void { ++observedCount; });
    auto observer = LifecycleObserver{std::move(subscription)};
    el::io::printLine("Moved-from handle active: "_el, subscription.isActive());

    // Discarding a returned handle cancels this registration at the end of the expression.
    (void)manager->events().addStateChanged([](const el::ApplicationPartManagerState) -> void {
        el::io::printLine("This discarded observer is never called."_el);
    });
    manager->prepare();
    (void)loop->runUntilIdle();
    el::io::printLine("Observed preparation transitions: "_el, observedCount);

    observer.stopWatching();
    observer.stopWatching();
    manager->start(); // With no parts, startup still produces manager lifecycle transitions.
    (void)loop->runUntilIdle();
    el::io::printLine("Count after cancellation: "_el, observedCount);
    el::io::printLine("Cancelled handle active: "_el, observer.isActive());

    manager.reset();
    el::io::printLine("Handle active after publisher destruction: "_el, diagnostic.isActive());
    if (const auto error = loop->takeError(); error != nullptr) {
        std::rethrow_exception(error);
    }
}

}
