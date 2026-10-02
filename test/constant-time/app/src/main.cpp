// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConstantTimeApplication.hpp"

auto main(const int argc, char *argv[]) -> int {
    auto application = app::constant_time::ConstantTimeApplication{argc, argv};
    return application.run();
}
