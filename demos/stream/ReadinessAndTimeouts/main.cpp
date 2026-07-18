// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadinessAndTimeoutsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CheckReadiness"_el, checkReadiness);
    app.registerDemo("RetryAtomicWrite"_el, retryAtomicWrite);
    app.registerDemo("RetryTimedOutRead"_el, retryTimedOutRead);
    app.registerDemo("WaitForInput"_el, waitForInput);
    return app.run();
}
