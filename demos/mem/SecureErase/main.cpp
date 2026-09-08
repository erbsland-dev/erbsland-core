// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SecureEraseDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("EraseDynamicBuffer"_el, eraseDynamicBuffer);
    app.registerDemo("EraseFixedStorage"_el, eraseFixedStorage);
    app.registerDemo("EraseRingBuffer"_el, eraseRingBuffer);
    app.registerDemo("EraseSharedBlocks"_el, eraseSharedBlocks);
    return app.run();
}
