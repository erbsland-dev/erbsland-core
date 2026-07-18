// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/CoGenerator_fwd.hpp"

#include <memory>
#include <vector>

namespace erbsland::re {

class Match32;

/// A shared pointer to a UTF-32 match result.
using Match32Ptr = std::shared_ptr<Match32>;

/// A generator returning UTF-32 matches.
using Match32Generator = util::CoGenerator<Match32Ptr>;

/// A list of UTF-32 matches.
using Match32List = std::vector<Match32Ptr>;

}
