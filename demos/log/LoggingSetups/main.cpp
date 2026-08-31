// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LoggingSetups.hpp"

#include <erbsland/all.hpp>

auto main(const int argc, char *argv[]) -> int {
    using namespace el::text::literals;
    if (argc >= 2) {
        const auto setup = el::String{argv[1]};
        if (setup == "application-configuration"_el) {
            return demo::runApplicationConfiguration(argc - 1, argv + 1);
        }
        if (setup == "deferred-configuration"_el) {
            return demo::runDeferredConfiguration(argc - 1, argv + 1);
        }
        if (setup == "minimal"_el) {
            return demo::runMinimalLogging(argc - 1, argv + 1);
        }
        if (setup == "medium"_el) {
            return demo::runMediumLogging(argc - 1, argv + 1);
        }
        if (setup == "large"_el) {
            return demo::runLargeLogging(argc - 1, argv + 1);
        }
    }
    return 64;
}
