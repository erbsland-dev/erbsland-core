// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Disassembler.hpp"

#include "../impl/diagnostics/Disassembler.hpp"

namespace erbsland::re::diagnostics {

Disassembler::Disassembler(const ConstRegExPtr &regEx) : _regEx{regEx} {
}

auto Disassembler::disassemble() const -> text::StringViewList {
    impl::Disassembler disassembler{_regEx->engine()->data()};
    return disassembler.disassemble();
}

}
