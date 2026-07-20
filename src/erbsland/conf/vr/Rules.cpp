// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Rules.hpp"

#include "../impl/vr/RulesBuilder.hpp"

namespace erbsland::conf::vr {

auto Rules::createFromDocument(const DocumentPtr &document) -> RulesPtr {
    auto builder = impl::RulesBuilder{};
    builder.readFromDocument(document);
    return builder.takeRules();
}

}
