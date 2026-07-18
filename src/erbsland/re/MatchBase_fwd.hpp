// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re {

class MatchBase;
/// A shared pointer to a match base result.
using MatchBasePtr = std::shared_ptr<MatchBase>;

}
