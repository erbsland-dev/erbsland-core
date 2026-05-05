// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// The string reader backend kind.
enum class StringReaderBackendKind : uint8_t { None, U8, U16, U32 };

}
