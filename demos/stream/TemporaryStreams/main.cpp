// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TemporaryStreamsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CleanupAfterAbort"_el, cleanupAfterAbort);
    app.registerDemo("CreateTemporaryBytes"_el, createTemporaryBytes);
    app.registerDemo("CreateTemporaryText"_el, createTemporaryText);
    app.registerDemo("ReleaseTemporaryFile"_el, releaseTemporaryFile);
    app.registerDemo("RemoveOnClose"_el, removeOnClose);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
