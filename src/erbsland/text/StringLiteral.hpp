// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringLiteral_fwd.hpp"

#include "u8/U8StringLiteral.hpp"

namespace erbsland::text {

/// The common string literal type.
using StringLiteral = U8StringLiteral<char>;

}
