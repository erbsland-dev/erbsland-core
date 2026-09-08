// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteBlockDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AccessingBytes"_el, accessingBytes);
    app.registerDemo("ComparingBlocks"_el, comparingBlocks);
    app.registerDemo("ConvertingBlocks"_el, convertingBlocks);
    app.registerDemo("CreatingBlocks"_el, creatingBlocks);
    app.registerDemo("EditingBlocks"_el, editingBlocks);
    app.registerDemo("InspectingContents"_el, inspectingContents);
    app.registerDemo("ManagingStorage"_el, managingStorage);
    app.registerDemo("ReadingIntegers"_el, readingIntegers);
    app.registerDemo("SlicingBlocks"_el, slicingBlocks);
    return app.run();
}
