// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyIndex.hpp"

namespace erbsland::conf::vr::builder {

void KeyIndex::apply(RuleDefinition &rule) const {
    rule.addKeyIndex(_name, _keyPaths, _caseSensitivity);
}

}
