// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RandomTopicsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ApiOverview"_el, apiOverview);
    app.registerDemo("ElementSampling"_el, elementSampling);
    app.registerDemo("RandomValues"_el, randomValues);
    app.registerDemo("SecureTokens"_el, secureTokens);
    return app.run();
}
