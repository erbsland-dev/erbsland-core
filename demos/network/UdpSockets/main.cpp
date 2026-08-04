// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "UdpSocketsDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/network/udp/all.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("PauseDelivery"_el, pauseDelivery, DemoApplication::Mode::EventLoop);
    return app.run();
}
