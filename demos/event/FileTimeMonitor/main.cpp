// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FileTimeMonitor.hpp"

namespace demo {

/// This simple main method is the entry point of the application.
auto main(const int argc, char *argv[]) -> int {
    auto app = FileTimeMonitorApp{argc, argv};
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
