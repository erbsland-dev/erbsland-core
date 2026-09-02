// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinimumVersion.hpp"

namespace erbsland::conf::vr::builder {

void MinimumVersion::apply(RuleDefinition &rule) const {
    rule.limitMinimumVersion(_version, _isNegated);
}

}
