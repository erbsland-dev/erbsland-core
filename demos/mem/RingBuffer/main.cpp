// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RingBufferDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CreatingBuffers"_el, creatingBuffers);
    app.registerDemo("ManagingCapacity"_el, managingCapacity);
    app.registerDemo("ReadingAndWritingBytes"_el, readingAndWritingBytes);
    app.registerDemo("ReadingAndWritingIntegers"_el, readingAndWritingIntegers);
    app.registerDemo("UnderstandingGrowth"_el, understandingGrowth);
    return app.run();
}
