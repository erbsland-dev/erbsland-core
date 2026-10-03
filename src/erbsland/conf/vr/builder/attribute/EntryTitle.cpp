// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EntryTitle.hpp"

#include "Title.hpp"

#include <memory>

namespace erbsland::conf::vr::builder {

void EntryTitle::apply(RuleDefinition &rule) const {
    rule.addEntryAttributes({std::make_shared<Title>(_title)});
}

}
