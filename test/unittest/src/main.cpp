// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "core/ApplicationTestScope.hpp"

#include <erbsland/unittest/UnitTest.hpp>

auto main(int argc, char *argv[]) -> int {
    ApplicationTestScopeBase::installApplicationInstanceManagerOverride();
    return erbsland::unittest::Controller::instance()->main(argc, argv);
};
