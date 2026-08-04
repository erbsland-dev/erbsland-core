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

el::HostLookupPtr lookup;

/// Resolve a public host name without blocking the event loop.
/// Keep the lookup alive until the final event runs; the lookup owns its stable event editor.
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
    app.enableTerminal();
    app.events()->invoke(resolveDocumentationHost);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
