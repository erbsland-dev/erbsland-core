// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkPart.hpp"

#include <erbsland/Application.hpp>

/// Register the component before run(); the default main starts its dedicated loop.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.registerPart<demo::WorkPart>();
    return app.run();
}
