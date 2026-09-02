// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Description.hpp"

namespace erbsland::conf::vr::builder {

void Description::apply(RuleDefinition &rule) const {
    rule.setDescription(_description);
}

}
