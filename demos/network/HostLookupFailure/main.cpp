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
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/PlatformErrorContext.hpp>

namespace demo {

using namespace el::text::literals;

struct LookupData {
    el::HostLookupPtr lookup;
} lookupData;

/// Resolver failures arrive through `onError()` as a structured context.
/// Throwing `NetworkError` transfers that context to the application's standard diagnostic boundary.
void resolveMissingHost() {
    const auto events = el::application().events();
    lookupData.lookup = events->get<el::Network>().createHostLookup();
    lookupData.lookup->events().onError(
        [](const el::NetworkErrorContext &context) -> void { throw el::NetworkError{context}; });
    lookupData.lookup->start(el::Host::fromStringOrThrow("stelling.invalid"_el));
}

auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.events()->invoke(resolveMissingHost);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
