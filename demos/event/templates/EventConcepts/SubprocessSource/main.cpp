// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProcessApplication.hpp"

/// Run the application after adapting its business operations and completion handling.
auto main(const int argc, char *argv[]) -> int {
    auto app = demo::ProcessApplication{argc, argv};
    return app.run();
}
