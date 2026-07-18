// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

using namespace demo;

auto main() -> int {
    auto value = el::SatInt32{10};
    el::io::printLine("Value: ", value);
    return 0;
}
