// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Work.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/EventTarget.hpp>

/// Queue startup during initialization and let the default main dispatch it.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    auto work = demo::Work{app};
    app.setInitializeFn([&app, &work]() -> void {
        // Queue startup; the default main enters dispatch later.
        app.loop()->invoke([&work]() -> void { work.begin(); });
    });
    return app.run();
}
