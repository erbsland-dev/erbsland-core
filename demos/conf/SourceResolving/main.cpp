// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SourceResolvingDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ConfigureFileResolver"_el, configureFileResolver);
    app.registerDemo("CustomResolver"_el, customResolver);
    app.registerDemo("DefaultFileResolver"_el, defaultFileResolver);
    return app.run();
}
