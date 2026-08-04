// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatAs_fwd.hpp"

#include "../core/Definitions.hpp"

namespace erbsland::text {

/// Override the default UTF-8 text representation for formatting.
///
/// Value types with `toString() const -> String` are automatically supported. Specialize this template for a custom
/// value type only when formatting requires a different representation.
/// @seedoc{/reference/text/string_formatter}
/// @tested{FormatAsTest}
template <typename T>
struct FormatAs;

}
