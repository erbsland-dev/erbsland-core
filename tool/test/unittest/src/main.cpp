// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>

auto main(const int argc, char *argv[]) -> int {
    return erbsland::unittest::Controller::instance()->main(argc, argv);
}
