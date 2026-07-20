// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesBuilder.hpp"

namespace erbsland::conf::vr {

void RulesBuilder::reset() {
    _builder.reset();
}

auto RulesBuilder::takeRules() -> RulesPtr {
    return _builder.takeRules();
}

}
