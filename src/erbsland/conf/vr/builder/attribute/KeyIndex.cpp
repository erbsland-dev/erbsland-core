// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyIndex.hpp"

#include "../NamePathHelper.hpp"

#include "../../../impl/vr/KeyDefinition.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void KeyIndex::operator()(impl::Rule &rule) {
    auto keys = detail::parseNamePathList(_keyPaths);
    auto definition = impl::KeyDefinition::create(_name, keys, _caseSensitivity, {});
    rule.addKeyDefinition(definition);
}

}
