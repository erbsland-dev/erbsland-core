// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CatalogServerPart.hpp"
#include "CatalogStoragePart.hpp"

namespace demo {

/// Register application parts before `run()` and let the default application main manage their lifecycle.
///
/// The application prepares the dependency graph, starts automatic parts, enters the main event loop, and coordinates
/// reverse dependency shutdown before returning from `run()`.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.info().setApplicationName("Catalog Service"_el);
    app.registerPart<CatalogStoragePart>();
    app.registerPart<CatalogServerPart>();
    auto stateSubscription = // Normally kept in the object that handles the event
        app.partManager()->addStateChanged([&app](const el::ApplicationPartManagerState state) -> void {
            if (state == el::ApplicationPartManagerState::Running) {
                el::io::printLine("all application parts are running"_el);
                app.quit();
            }
        });
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
