// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::re::impl {

/// Options controlling regular-expression disassembly.
enum class DisassemblerFlag : uint8_t {
    None = 0,

    /// Output the disassembly in a format that can be used in unit tests.
    /// - Outputs no comments.
    /// - Outputs no labels.
    /// - Outputs no hexadecimal codes.
    /// - Does not align the output.
    TestOutput = 1U << 0U,
};

}
