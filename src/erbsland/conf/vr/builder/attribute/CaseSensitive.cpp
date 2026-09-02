// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaseSensitive.hpp"

namespace erbsland::conf::vr::builder {

void CaseSensitive::apply(RuleDefinition &rule) const {
    rule.setCaseSensitivity(_caseSensitivity);
}

}
