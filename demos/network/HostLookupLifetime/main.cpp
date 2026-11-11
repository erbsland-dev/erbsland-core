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
#include <erbsland/Network.hpp>

namespace demo {

using namespace el::text::literals;

el::HostLookupPtr lookup;

/// Store the lookup in the object that needs its result and configure its source-owned editor directly.
/// The lookup owns its handlers and can be released safely from its final handler.
void resolveAndRelease() {
    const auto events = el::application().loop();
    lookup = events->get<el::Network>().createHostLookup();
    lookup->events()
        .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
            el::stdOut()->printLine("Resolved address: "_el, addresses.first());
        })
        .onFinal([]() -> void {
            lookup.reset();
            el::stdOut()->printLine("Lookup released from its final handler."_el);
            el::application().quit();
        });
    lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.loop()->invoke(resolveAndRelease);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
