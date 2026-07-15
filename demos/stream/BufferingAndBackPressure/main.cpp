// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BufferingAndBackPressureDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ApplyBackPressure"_el, applyBackPressure);
    app.registerDemo("ConfigureBuffers"_el, configureBuffers);
    app.registerDemo("FlushAcceptedOutput"_el, flushAcceptedOutput);
    app.registerDemo("ReadBoundedChunks"_el, readBoundedChunks);
    app.registerDemo("RejectOversizedRequest"_el, rejectOversizedRequest);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
