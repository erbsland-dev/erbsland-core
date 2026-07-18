// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CustomStreamsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ReadCustomStream"_el, readCustomStream);
    app.registerDemo("UseCustomStreamAsynchronously"_el, useCustomStreamAsynchronously);
    app.registerDemo("VerifyCustomContracts"_el, verifyCustomContracts);
    app.registerDemo("WriteCustomStream"_el, writeCustomStream);
    return app.run();
}
