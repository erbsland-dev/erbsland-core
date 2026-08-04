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
#include <erbsland/network/source/NetworkError.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/system/PlatformErrorContext.hpp>

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
    app.enableTerminal();
    app.events()->invoke(resolveMissingHost);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
