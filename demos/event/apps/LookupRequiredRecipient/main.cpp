// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LookupRequiredRecipientApp.hpp"

/// Run the server-address lookup and complete cleanup before returning.
auto main(const int argc, char *argv[]) -> int {
    auto app = demo::LookupRequiredRecipientApp{argc, argv};
    const auto result = app.run();
    app.printCleanupResult();
    return result;
}
