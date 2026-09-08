// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteArrayDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AccessingBytes"_el, accessingBytes);
    app.registerDemo("BorrowingRanges"_el, borrowingRanges);
    app.registerDemo("ChangingBytes"_el, changingBytes);
    app.registerDemo("ChangingRanges"_el, changingRanges);
    app.registerDemo("CopyingFromSpan"_el, copyingFromSpan);
    app.registerDemo("GrowingIntoBuffer"_el, growingIntoBuffer);
    app.registerDemo("MovingBits"_el, movingBits);
    return app.run();
}
