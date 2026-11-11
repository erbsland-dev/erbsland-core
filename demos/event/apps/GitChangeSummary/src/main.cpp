// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "GitChangeSummaryApp.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::GitChangeSummaryApp{argc, argv};
    return app.run();
}
