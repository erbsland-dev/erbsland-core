// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeClientApp.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::TimeClientApp{argc, argv};
    app.enableTerminal();
    return app.run();
}
