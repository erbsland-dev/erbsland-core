// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSourceDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventScheduler.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/StandardStreams.hpp>

#include <exception>

namespace demo {

/// Configure handlers on the owner loop before starting an asynchronous source.
/// Each on-method replaces one handler; an empty callback clears it without cancelling the operation.
void handlerSetup() {
    const auto loop = el::EventLoop::create();
    auto lookup = el::HostLookupPtr{};
    auto savedAddresses = el::List<el::IpAddress>{};
    loop->invoke([&lookup, &savedAddresses]() -> void {
        const auto target = el::currentEvents();
        auto &network = target->get<el::Network>();
        auto &scheduler = target->get<el::EventScheduler>();
        lookup = network.createHostLookup();
        auto &editor = lookup->events();
        editor.onResolved([](const el::List<el::IpAddress> &) -> void {
            el::io::printLine("This replaced handler is never called."_el);
        });
        editor
            .onResolved([&savedAddresses](const el::List<el::IpAddress> &addresses) -> void {
                savedAddresses = addresses; // Copy the callback's borrowed result for later use.
                el::io::printLine("Replacement handler: "_el, addresses.first());
            })
            .onError([](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
            })
            .onFinal([]() -> void { el::io::printLine("Operation finished."_el); });

        // Generic editor code can deliberately retain the source and its callback target.
        const auto retainedSource = editor.source();
        const auto callbackTarget = editor.target();
        el::io::printLine("Editor target matches source owner: "_el, callbackTarget == retainedSource->ownerEvents());
        scheduler.invokeAfter(el::Milliseconds{0}, [caption = "Skovens lys"_el]() -> void {
            el::io::printLine("Illustration caption: "_el, caption);
        });
        lookup->start(el::Host::fromStringOrThrow("127.0.0.1"_el));
    });
    (void)loop->runUntilIdle();
    el::io::printLine("Saved result: "_el, savedAddresses.first());

    // Reuse the source, keeping its final handler but clearing the result handler.
    loop->invoke([lookup]() -> void {
        lookup->events().onResolved({});
        lookup->start(el::Host::fromStringOrThrow("127.0.0.2"_el));
    });
    (void)loop->runUntilIdle();
    if (const auto error = loop->takeError(); error != nullptr) {
        std::rethrow_exception(error);
    }
}

}
