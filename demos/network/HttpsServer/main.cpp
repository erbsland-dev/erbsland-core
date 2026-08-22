// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HttpsServerApp.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::HttpsServerApp{argc, argv};
    app.enableTerminal();
    return app.run();
}
