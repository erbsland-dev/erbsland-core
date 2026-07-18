// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Disassembler.hpp"

#include "../impl/diagnostics/Disassembler.hpp"
#include "../impl/engine/Engine.hpp"
#include "../RegEx.hpp"

#include "../../text/StringList.hpp"

namespace erbsland::re::diagnostics {

Disassembler::Disassembler(const ConstRegExPtr &regEx) : _regEx{regEx} {
}

auto Disassembler::disassemble() const -> text::StringList {
    impl::Disassembler disassembler{_regEx->engine()->data()};
    return disassembler.disassemble();
}

}
