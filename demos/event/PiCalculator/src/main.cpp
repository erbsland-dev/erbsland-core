// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PiCalculatorApp.hpp"

/// Run an exact, cancellable calculation with optional terminal presentation.
auto main(const int argc, char *argv[]) -> int {
    auto app = demo::PiCalculatorApp{argc, argv};
    return app.run();
}
