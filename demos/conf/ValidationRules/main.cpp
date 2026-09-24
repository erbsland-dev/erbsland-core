// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidationRulesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BuiltRules"_el, builtRules);
    app.registerDemo("EmbeddedRules"_el, embeddedRules);
    return app.run();
}
