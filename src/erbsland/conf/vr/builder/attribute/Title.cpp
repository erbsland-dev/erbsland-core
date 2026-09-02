// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Title.hpp"

namespace erbsland::conf::vr::builder {

void Title::apply(RuleDefinition &rule) const {
    rule.setTitle(_title);
}

}
