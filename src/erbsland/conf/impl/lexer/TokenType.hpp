// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../syntax/TokenKind.hpp"

namespace erbsland::conf::impl {

/// The shared token classification used by the lexer and syntax snapshots.
using TokenType = syntax::TokenKind;

}
