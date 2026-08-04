// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::mem::impl {

/// Throw the strict fixed-array source-length error outside the `ByteArray` template.
/// @throws err::ParameterError Always.
/// @tested{ByteArrayTest}
[[noreturn]] void throwByteArrayWrongLength();

}
