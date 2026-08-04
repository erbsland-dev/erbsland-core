// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include "core/ApplicationTestScope.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>
#include <iostream>

auto main(const int argc, char *argv[]) -> int {
    ApplicationTestScope<>::installApplicationInstanceManagerOverride();
    try {
        auto environment = InteropEnvironment{std::filesystem::path{argv[0]}};
        InteropEnvironment::install(environment);
        return erbsland::unittest::Controller::instance()->main(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "Failed to initialize network interop tests: " << error.what() << '\n';
        return 1;
    }
}
