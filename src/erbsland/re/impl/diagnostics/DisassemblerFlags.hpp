// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::re::impl {

enum class DisassemblerFlag : uint8_t {
    None = 0,

    /// Output the disassembly in a format that can be used in unit tests.
    /// - Outputs no comments.
    /// - Outputs no labels.
    /// - Outputs no hexadecimal codes.
    /// - Does not align the output.
    TestOutput = 1U << 0U,
};

/// Flags controlling diagnostic disassembly output.
/// @tested{DisassemblerTest}
class DisassemblerFlags : public util::EnumFlags<DisassemblerFlag, DisassemblerFlags> {
    using Base = util::EnumFlags<DisassemblerFlag, DisassemblerFlags>;

public:
    using Base::Base;
};

}
