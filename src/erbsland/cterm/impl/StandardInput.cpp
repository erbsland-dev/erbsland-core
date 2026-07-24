// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardInput.hpp"

#include "../../stream/StandardStreams.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringSide.hpp"
#include "../../unit/CpLength.hpp"

namespace erbsland::cterm::impl {

using namespace text::literals;

auto readStandardInputLine() -> text::String {
    auto result = stream::stdIn()->readLine();
    if (!result.hasData()) {
        return {};
    }
    auto line = result.takeData();
    if (line.endsWith("\r\n"_el)) {
        return line.slice(text::StringSide::Front, line.characterLength() - unit::CpLength{2U});
    }
    if (line.endsWith("\r"_el) || line.endsWith("\n"_el)) {
        return line.slice(text::StringSide::Front, line.characterLength() - unit::CpLength::one());
    }
    return line;
}

}
