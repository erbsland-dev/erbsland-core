// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LexerToken.hpp"

#include "../../../util/CoGenerator.hpp"

namespace erbsland::conf::impl {

/// A token generator
using TokenGenerator = util::CoGenerator<LexerToken>;

}
