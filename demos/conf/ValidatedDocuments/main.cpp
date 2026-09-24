// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidatedDocumentsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("DefaultsAndValidation"_el, defaultsAndValidation);
    app.registerDemo("RuleMetadata"_el, ruleMetadata);
    app.registerDemo("SecretValues"_el, secretValues);
    return app.run();
}
