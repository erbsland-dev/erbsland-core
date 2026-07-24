// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StoryDemo.hpp"

using namespace demo;

auto main(const int argc, char **argv) -> int {
    StoryDemo app{argc, argv};
    return app.run();
}
