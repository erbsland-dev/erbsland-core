// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_network.hpp>
#include <erbsland/all_network_host_lookup.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/Network.hpp>

namespace demo {

using namespace el::text::literals;

struct LookupData {
    el::ManagedEventThreadPtr eventThread;
    el::HostLookupPtr lookup;
} lookupData;

/// Create and start a lookup in the event loop that shall own it.
/// Its handlers run in that same loop, so other application state can stay confined to the chosen event thread.
void resolveInWorkerLoop() {
    const auto workerEvents = el::currentLoop();
    lookupData.lookup = workerEvents->get<el::Network>().createHostLookup();
    lookupData.lookup->events()
        .onResolved([workerEvents](const el::List<el::IpAddress> &) -> void {
            const auto runsInWorker = el::currentLoop() == workerEvents;
            el::stdOut()->printLine("Handler runs in worker event loop: "_el, runsInWorker);
        })
        .onFinal([]() -> void { el::application().quit(); });
    lookupData.lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    lookupData.eventThread = app.createEventThread();
    lookupData.eventThread->start();
    lookupData.eventThread->loop()->invoke(resolveInWorkerLoop);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
