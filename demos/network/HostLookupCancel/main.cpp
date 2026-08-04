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

el::HostLookupPtr lookup;

/// Cancel a lookup when its result is no longer useful.
/// Cancellation is safe to repeat and prevents a pending completion handler from running when it wins the race.
void cancelUnneededLookup() {
    const auto events = el::application().events();
    lookup = events->get<el::Network>().createHostLookup();
    lookup->events()
        .onResolved([](const el::List<el::IpAddress> &) -> void {
            el::stdOut()->printLine("The cancelled lookup unexpectedly completed."_el);
        })
        .onFinal([]() -> void {
            el::stdOut()->printLine("Lookup state after final event: Inactive"_el);
            el::application().quit();
        });

    lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
    lookup->cancel();
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.events()->invoke(cancelUnneededLookup);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
