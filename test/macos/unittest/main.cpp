// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/unittest/UnitTest.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto application = erbsland::core::Application{argc, argv};
    return erbsland::unittest::Controller::instance()->main(argc, argv);
}
