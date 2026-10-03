// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ServerPart.hpp"

#include <erbsland/Application.hpp>

namespace demo {

/// Register an application part and let the default main manage its event thread and shutdown.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.info().setApplicationName("File Server"_el);
    app.enableServiceLifecycle();
    app.registerPart<ServerPart>();
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
