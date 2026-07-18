// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/CoGenerator_fwd.hpp"

#include <memory>
#include <vector>

namespace erbsland::re {

class Match16;
/// A shared pointer to a UTF-16 match result.
using Match16Ptr = std::shared_ptr<Match16>;

/// A generator returning UTF-16 matches.
using Match16Generator = util::CoGenerator<Match16Ptr>;

/// A list of UTF-16 matches.
using Match16List = std::vector<Match16Ptr>;

}
