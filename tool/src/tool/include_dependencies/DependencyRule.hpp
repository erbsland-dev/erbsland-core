// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/conf/Value.hpp>
#include <erbsland/StringHashSet.hpp>

namespace tool::include_dependencies {

/// One validated required-dependency rule with retained configuration context.
struct DependencyRule {
    el::String name;                  ///< User-visible unique rule name.
    el::String target;                ///< Canonical dependency target after inventory validation.
    el::StringHashSet excluded;       ///< Canonical headers exempt from this rule.
    el::conf::ValuePtr configuration; ///< Original section for contextual diagnostics.
};

}
