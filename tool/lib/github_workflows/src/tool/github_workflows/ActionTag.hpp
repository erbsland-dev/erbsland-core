// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StableTag.hpp"

namespace tool::github_workflows {

/// A stable tag paired with its peeled commit or lightweight target.
struct ActionTag {
    StableTag version; ///< Exact stable version tag.
    el::String sha;    ///< Lowercase full commit SHA.
};

}
