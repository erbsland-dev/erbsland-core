// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Disassembler_fwd.hpp"

#include "../RegEx_fwd.hpp"

#include "../../text/StringList_fwd.hpp"

namespace erbsland::re::diagnostics {

/// A disassembler for the regular expression engine data.
class Disassembler {
public:
    /// Create a new instance for the given regular expression.
    explicit Disassembler(const ConstRegExPtr &regEx);

public:
    /// Disassemble the engine data into human-readable instructions.
    [[nodiscard]] auto disassemble() const -> text::StringList;

private:
    ConstRegExPtr _regEx;
};

}
