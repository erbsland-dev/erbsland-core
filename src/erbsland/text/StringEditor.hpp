// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringEditor_fwd.hpp"

#include "u8/U8StringEditor.hpp"

namespace erbsland::text {

/// The common UTF-8 editor for explicit in-place editing and local construction tasks.
/// Use `String` for parameters, read-only storage, and ordinary copy-returning transformations.
using StringEditor = U8StringEditor;

}
