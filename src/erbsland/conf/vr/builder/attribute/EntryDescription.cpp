// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EntryDescription.hpp"

#include "Description.hpp"

#include <memory>

namespace erbsland::conf::vr::builder {

void EntryDescription::apply(RuleDefinition &rule) const {
    rule.addEntryAttributes({std::make_shared<Description>(_description)});
}

}
