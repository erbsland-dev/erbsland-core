// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSourceDemos.hpp"
#include "IllustrationController.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>
#include <memory>

namespace demo {

/// Value captures preserve input; a strong pointer capture keeps a recipient alive until queued work finishes.
/// @notest{Compiled documentation demo.}
void retainingValues() {
    const auto loop = el::EventLoop::create();
    auto caption = "Skovens lys"_el;
    loop->invoke([caption]() -> void { el::io::printLine("Captured value: "_el, caption); });
    caption = "Havets farver"_el; // This change cannot alter the captured value.

    auto retainedCaption = std::make_shared<el::String>(caption);
    loop->invoke([retainedCaption]() -> void { el::io::printLine("Retained object: "_el, *retainedCaption); });
    retainedCaption.reset(); // The queued callback still owns the object.
    (void)loop->runUntilIdle();
}

/// Weak captures let the recipient disappear while independently retained work finishes.
/// Keeping the source alive is a separate decision from keeping its recipient alive.
void weakController() {
    retainingValues();
    const auto loop = el::EventLoop::create();
    auto controller = std::shared_ptr<IllustrationController>{};
    auto abandonedSource = el::HostLookupPtr{};
    loop->invoke([&controller, &abandonedSource]() -> void {
        const auto target = el::currentEvents();
        controller = std::make_shared<IllustrationController>(target, "Skovens lys"_el);
        controller->start(el::Host::fromStringOrThrow("127.0.0.1"_el));

        auto temporary = std::make_shared<IllustrationController>(target, "Havets farver"_el);
        abandonedSource = temporary->lookup();
        temporary->start(el::Host::fromStringOrThrow("127.0.0.2"_el));
        temporary.reset(); // The source survives, but its weak callback cannot access the recipient.
    });
    (void)loop->runUntilIdle();
    if (const auto error = loop->takeError(); error != nullptr) {
        std::rethrow_exception(error);
    }
}

}
