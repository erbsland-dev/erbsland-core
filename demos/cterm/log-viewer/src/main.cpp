// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LogViewerApp.hpp"

auto main(const int argc, char **argv) -> int {
    LogViewerApp app{argc, argv};
    return app.run();
}
