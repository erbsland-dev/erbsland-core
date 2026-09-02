// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MaximumVersion.hpp"

namespace erbsland::conf::vr::builder {

void MaximumVersion::apply(RuleDefinition &rule) const {
    rule.limitMaximumVersion(_version, _isNegated);
}

}
