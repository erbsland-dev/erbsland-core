// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteSpan.hpp"

namespace erbsland::mem {

/// Secure erase the memory from a byte span.
/// The platform implementation prevents the erase from being optimized away.
/// @param span The writable bytes to erase.
/// @tested{SecureEraseTest}
void secureErase(ByteSpan span) noexcept;

}
