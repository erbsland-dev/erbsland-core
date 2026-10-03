// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSourceDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("HandlerSetup"_el, handlerSetup);
    app.registerDemo("ObserverLifetime"_el, observerLifetime);
    app.registerDemo("WeakController"_el, weakController);
    return app.run();
}
