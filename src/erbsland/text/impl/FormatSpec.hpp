// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatSpecTypes.hpp"

#include <variant>

namespace erbsland::text::impl {

/// Parsed format specification for one replacement field.
using FormatSpec = std::
    variant<LegacyFormatSpec, NamedTextFormatSpec, NamedNumberFormatSpec, NamedBooleanFormatSpec, NamedBytesFormatSpec>;

}
