// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IsOptional.hpp"

namespace erbsland::conf::vr::builder {

void IsOptional::apply(RuleDefinition &rule) const {
    rule.setOptional(_isOptional);
}

}
