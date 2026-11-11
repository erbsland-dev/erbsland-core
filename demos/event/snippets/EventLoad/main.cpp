// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventLoadDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ChunkedWork"_el, chunkedWork);
    app.registerDemo("CoalescedUpdates"_el, coalescedUpdates);
    app.registerDemo("CollectReadings"_el, collectReadings);
    app.registerDemo("ReplaceableUpdates"_el, replaceableUpdates);
    return app.run();
}
