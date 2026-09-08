// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteBufferDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AccessingBytes"_el, accessingBytes);
    app.registerDemo("ConvertingBuffers"_el, convertingBuffers);
    app.registerDemo("CreatingBuffers"_el, creatingBuffers);
    app.registerDemo("ManagingStorage"_el, managingStorage);
    app.registerDemo("ManipulatingRanges"_el, manipulatingRanges);
    app.registerDemo("ReadingAndWritingIntegers"_el, readingAndWritingIntegers);
    return app.run();
}
