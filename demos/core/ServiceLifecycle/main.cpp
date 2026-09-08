// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CatalogMonitorApplication.hpp"

namespace demo {

/// Enable native service integration before the application lifecycle starts.
///
/// The executable runs as a Windows service when the Service Control Manager launches it.
/// Everywhere else, it runs as an ordinary foreground process.
/// Both modes use the same initialization, main event loop, and orderly cleanup.
auto main(const int argc, char *argv[]) -> int {
    auto app = CatalogMonitorApplication{argc, argv};
    app.enableServiceLifecycle();
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
