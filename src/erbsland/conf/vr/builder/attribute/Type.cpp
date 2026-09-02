// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Type.hpp"

namespace erbsland::conf::vr::builder {

void Type::apply(RuleDefinition &rule) const {
    rule.setType(_type);
}

}
