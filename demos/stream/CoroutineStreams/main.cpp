// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CoroutineStreamsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AwaitByteRead"_el, awaitByteRead);
    app.registerDemo("AwaitTextWrite"_el, awaitTextWrite);
    app.registerDemo("CancelPendingTask"_el, cancelPendingTask);
    app.registerDemo("HandleCoroutineTimeout"_el, handleCoroutineTimeout);
    app.registerDemo("ProcessBlockGenerator"_el, processBlockGenerator);
    app.registerDemo("ProcessLineGenerator"_el, processLineGenerator);
    app.registerDemo("RetainStreamOwnership"_el, retainStreamOwnership);
    return app.run();
}
