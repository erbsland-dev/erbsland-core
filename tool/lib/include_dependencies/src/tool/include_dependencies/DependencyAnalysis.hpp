// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringList.hpp>

namespace tool::include_dependencies {

/// Missing public dependencies and one suggested insertion per leaf component.
struct DependencyAnalysis {
    el::StringList missing;    ///< All public headers that cannot reach the target.
    el::StringList insertions; ///< Minimal deterministic public insertion frontier.
};

}
