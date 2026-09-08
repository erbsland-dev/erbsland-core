// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteIntegerFormatDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("FixedFormats"_el, fixedFormats);
    app.registerDemo("SignedVariableLength"_el, signedVariableLength);
    app.registerDemo("UnsignedBase128"_el, unsignedBase128);
    app.registerDemo("UnsignedVariableLength"_el, unsignedVariableLength);
    return app.run();
}
