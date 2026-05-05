// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringCharViewDemos.hpp"

#include <DemoCommon.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CharacterGrid"_el, characterGrid);
    return app.run();
}
