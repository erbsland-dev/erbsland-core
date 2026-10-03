// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EntryType.hpp"

namespace erbsland::conf::vr::builder {

void EntryType::apply(RuleDefinition &rule) const {
    rule.setEntryType(_type);
}

}
