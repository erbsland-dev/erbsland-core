// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "NetworkValuesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ParseAddressesAndHosts"_el, parseAddressesAndHosts);
    app.registerDemo("BuildEndpoints"_el, buildEndpoints);
    app.registerDemo("MatchNetworks"_el, matchNetworks);
    return app.run();
}
