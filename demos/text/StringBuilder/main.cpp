// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringBuilderDemos.hpp"

#include <DemoCommon.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("FieldGuideCards"_el, fieldGuideCards);
    return app.run();
}
