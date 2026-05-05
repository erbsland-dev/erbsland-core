// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringEncoderDemos.hpp"

#include <DemoCommon.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BomHandling"_el, bomHandling);
    app.registerDemo("ByteOrder"_el, byteOrder);
    app.registerDemo("EncodeStrings"_el, encodeStrings);
    return app.run();
}
