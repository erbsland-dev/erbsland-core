// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteStreamsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BuildRecord"_el, buildRecord);
    app.registerDemo("NavigateReader"_el, navigateReader);
    app.registerDemo("PositionWriter"_el, positionWriter);
    app.registerDemo("ReadBytes"_el, readBytes);
    app.registerDemo("ReadIntegers"_el, readIntegers);
    app.registerDemo("ReadRecord"_el, readRecord);
    app.registerDemo("ReadText"_el, readText);
    app.registerDemo("StorageManagement"_el, storageManagement);
    app.registerDemo("WriteBytes"_el, writeBytes);
    app.registerDemo("WriteIntegers"_el, writeIntegers);
    app.registerDemo("WriteText"_el, writeText);
    return app.run();
}
