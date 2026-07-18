// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DisplayAllColorsApp.hpp"

using namespace demo;

auto main(const int argc, char **argv) -> int {
    DisplayAllColorsApp app{argc, argv};
    return app.run();
}
