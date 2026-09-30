// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SignedByteBlocksDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CreateKeys"_el, createKeys);
    app.registerDemo("SignAndVerify"_el, signAndVerify);
    app.registerDemo("VerifyRecord"_el, verifyRecord);
    return app.run();
}
