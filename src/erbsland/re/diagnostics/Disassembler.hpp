// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../RegEx.hpp"

#include "../../text/StringViewList.hpp"

namespace erbsland::re::diagnostics {

/// A disassembler for the regular expression engine data.
class Disassembler {
public:
    /// Create a new instance for the given regular expression.
    explicit Disassembler(const ConstRegExPtr &regEx);

public:
    /// Disassemble the engine data into human-readable instructions.
    [[nodiscard]] auto disassemble() const -> text::StringViewList;

private:
    ConstRegExPtr _regEx;
};

}
