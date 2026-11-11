// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkApplication.hpp"

/// Run the complete application lifecycle with the default event-loop main.
auto main(const int argc, char *argv[]) -> int {
    auto app = demo::WorkApplication{argc, argv};
    return app.run();
}
