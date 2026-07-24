// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto application = app::regex::RegexApiProfileApplication{argc, argv};
    return application.run();
}
