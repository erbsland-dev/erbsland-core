// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Assembler_fwd.hpp"

#include "../RegEx_fwd.hpp"

#include "../../text/StringList_fwd.hpp"

namespace erbsland::re::diagnostics {

/// An assembler for diagnostics, unit test, and experiments.
/// Please read the documentation for the full syntax of the assembler language.
class Assembler {
public:
    // defaults
    Assembler() = default;
    ~Assembler() = default;

public:
    /// Compile the given assembler program into a regular expression object.
    /// @param lines The lines of the assembler code to compile.
    /// @return The compiled regular expression object.
    /// @throws RegExError on any compilation error.
    [[nodiscard]] auto compile(const text::StringList &lines) const -> RegExPtr;
};

}
