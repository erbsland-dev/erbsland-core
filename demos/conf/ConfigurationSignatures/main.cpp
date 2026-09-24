// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationSignaturesDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("SignDocuments"_el, signDocuments);
    app.registerDemo("ValidateSignatures"_el, validateSignatures);
    return app.run();
}
