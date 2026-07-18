// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LifecycleDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AbortPendingOutput"_el, abortPendingOutput);
    app.registerDemo("CloseGracefully"_el, closeGracefully);
    app.registerDemo("ContinueCloseAfterTimeout"_el, continueCloseAfterTimeout);
    app.registerDemo("ObserveFailedState"_el, observeFailedState);
    return app.run();
}
