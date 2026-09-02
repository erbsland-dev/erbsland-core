// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfVersion.hpp"

namespace erbsland::conf::vr::builder {

void ConfVersion::apply(RuleDefinition &rule) const {
    rule.limitVersions(_versions, _isNegated);
}

}
