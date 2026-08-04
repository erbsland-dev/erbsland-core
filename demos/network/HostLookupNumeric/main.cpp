// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_network.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/network/host_lookup/all.hpp>
#include <erbsland/network/host_lookup/HostLookup.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>

namespace demo {

using namespace el::text::literals;

struct LookupData {
    el::HostLookupPtr lookup;
} lookupData;

/// Numeric hosts use the same asynchronous callback contract as names.
/// The public address is returned on a later event-loop turn without calling the native resolver or contacting it.
void resolveMoonRelay() {
    const auto events = el::application().events();
    lookupData.lookup = events->get<el::Network>().createHostLookup();
    lookupData.lookup->events()
        .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
            el::stdOut()->printLine("Moon relay (Luna crescente): "_el, addresses.first());
        })
        .onError([](const el::NetworkErrorContext &error) -> void {
            el::stdErr()->printLine("Lookup failed: "_el, error.title());
            el::application().quit(el::ExitCode::failure());
        })
        .onFinal([]() -> void { el::application().quit(); });
    lookupData.lookup->start(el::Host::fromStringOrThrow("9.9.9.9"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableTerminal();
    app.events()->invoke(resolveMoonRelay);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
