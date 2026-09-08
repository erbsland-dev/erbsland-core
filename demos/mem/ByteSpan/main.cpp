// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteSpanDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BorrowedViews"_el, borrowedViews);
    app.registerDemo("IntegerAccess"_el, integerAccess);
    app.registerDemo("SpanConversions"_el, spanConversions);
    return app.run();
}
