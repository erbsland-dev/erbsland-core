// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ServerPart.hpp"
#include "ServerStatus.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationPartIdentifier.hpp>
#include <erbsland/ApplicationPartManager.hpp>
#include <erbsland/ApplicationPartManagerEventEditor.hpp>
#include <erbsland/ApplicationPartState.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

/// Register an application part and let the default main manage its event thread and shutdown.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.info().setApplicationName("File Server"_el);
    app.enableServiceLifecycle();
    app.registerPart<ServerPart>();
    // Read a statistics snapshot on the application thread while the part is stopping.
    const auto stateSubscription = app.partManager()->events().addPartStateChanged(
        [&app](el::ApplicationPartIdentifierPtr, const el::ApplicationPartState state) -> void {
            if (state == el::ApplicationPartState::Stopping) {
                const auto status = app.part<ServerStatus>();
                el::io::printLine("Directory-index requests at shutdown: "_el, status->indexRequestCount());
            }
        });
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
