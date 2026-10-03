// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_network.hpp>
#include <erbsland/all_network_host_lookup.hpp>
#include <erbsland/all_network_source.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>

namespace demo {

using namespace el::text::literals;

/// Keep the lookup alive after its setup function returns.
el::HostLookupPtr lookup;

/// Resolve a public host name without blocking the event loop.
/// A host can resolve to several addresses, so successful code processes the complete result list.
void resolveDocumentationHost() {
    const auto events = el::application().events();
    lookup = events->get<el::Network>().createHostLookup();
    lookup->events()
        .onResolved([](const el::List<el::IpAddress> &addresses) -> void {
            el::stdOut()->printLine("core.erbsland.dev resolved to:"_el);
            for (const auto &address : addresses) {
                el::stdOut()->printLine("  "_el, address);
            }
        })
        .onError([](const el::NetworkErrorContext &error) -> void {
            el::stdErr()->printLine("Lookup failed: "_el, error.title());
            el::application().quit(el::ExitCode::failure());
        })
        .onFinal([]() -> void { el::application().quit(); });
    lookup->start(el::Host::fromStringOrThrow("core.erbsland.dev"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.events()->invoke(resolveDocumentationHost);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
