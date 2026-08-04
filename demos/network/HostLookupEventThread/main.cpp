// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_network.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/network/host_lookup/all.hpp>
#include <erbsland/network/host_lookup/HostLookup.hpp>

namespace demo {

using namespace el::text::literals;

struct LookupData {
    el::ManagedEventThreadPtr eventThread;
    el::HostLookupPtr lookup;
} lookupData;

/// Create and start a lookup in the event loop that shall own it.
/// Its handlers run in that same loop, so other application state can stay confined to the chosen event thread.
void resolveInWorkerLoop() {
    const auto workerEvents = el::currentEvents();
    lookupData.lookup = workerEvents->get<el::Network>().createHostLookup();
    lookupData.lookup->events()
        .onResolved([workerEvents](const el::List<el::IpAddress> &) -> void {
            const auto runsInWorker = el::currentEvents() == workerEvents;
            el::stdOut()->printLine("Handler runs in worker event loop: "_el, runsInWorker);
        })
        .onFinal([]() -> void { el::application().quit(); });
    lookupData.lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    lookupData.eventThread = app.createEventThread();
    lookupData.eventThread->start();
    lookupData.eventThread->events()->invoke(resolveInWorkerLoop);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
