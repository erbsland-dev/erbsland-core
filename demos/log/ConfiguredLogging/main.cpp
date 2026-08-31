// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfiguredLoggingApp.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::ConfiguredLoggingApp{argc, argv};
    return app.run();
}
