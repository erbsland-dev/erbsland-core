// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TcpMapClientApp.hpp"

#include <DemoTerminal.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::TcpMapClientApp{argc, argv};
    demo::DemoTerminal::enable(app);
    return app.run();
}
