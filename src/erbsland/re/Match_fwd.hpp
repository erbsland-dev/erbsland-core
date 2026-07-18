// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/CoGenerator_fwd.hpp"

#include <memory>
#include <vector>

namespace erbsland::re {

class Match;
/// A shared pointer to a match result.
using MatchPtr = std::shared_ptr<Match>;

/// A generator returning matches.
using MatchGenerator = util::CoGenerator<MatchPtr>;

/// A list of matches.
using MatchList = std::vector<MatchPtr>;

}
