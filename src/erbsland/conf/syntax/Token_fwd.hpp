// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf::syntax {

class Token;
/// Shared ownership of a syntax token.
using TokenPtr = std::shared_ptr<Token>;

}
