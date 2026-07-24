// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HashAlgorithmsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("FixedSha3"_el, fixedSha3);
    app.registerDemo("HashBoundedInput"_el, hashBoundedInput);
    app.registerDemo("PersistAlgorithm"_el, persistAlgorithm);
    app.registerDemo("SelectAlgorithm"_el, selectAlgorithm);
    app.registerDemo("StoreDigest"_el, storeDigest);
    return app.run();
}
