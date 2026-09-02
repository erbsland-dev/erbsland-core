// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/List_fwd.hpp"

namespace erbsland::text::fuzzy {

class Match;

/// A ranked list of fuzzy text matches.
using MatchList = util::List<Match>;

}
