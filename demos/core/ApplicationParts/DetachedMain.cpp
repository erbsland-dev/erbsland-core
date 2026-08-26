// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CatalogServerPart.hpp"
#include "CatalogStoragePart.hpp"

#include <exception>

namespace demo {

/// Run application parts independently when no `Application` lifecycle should own them.
///
/// A detached manager owns its control event thread, while every started part receives its own event thread.
/// The caller explicitly prepares, starts, waits for, uses, and stops the service graph.
auto main() -> int {
    auto manager = el::ApplicationPartManager::create();
    manager->registerPart<CatalogStoragePart>();
    manager->registerPart<CatalogServerPart>();
    manager->prepare();
    manager->start();
    if (!manager->waitForRunning()) {
        if (manager->hasError()) {
            std::rethrow_exception(manager->takeError());
        }
        return el::ExitCode::failure().toRawValue();
    }

    const auto server = manager->part<CatalogServer>();
    el::io::printLine("using "_el, server->endpoint());

    manager->stop();
    if (!manager->waitForStopped() && manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
    return el::ExitCode::success().toRawValue();
}

}

auto main() -> int {
    return demo::main();
}
