// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkApplication.hpp"

/// Run a procedural application with one managed worker loop.
auto main(const int argc, char *argv[]) -> int {
    auto app = demo::WorkApplication{argc, argv};
    return app.run();
}
