// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalEmulationOptions.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

namespace erbsland::cterm {

using namespace text::literals;

auto TerminalEmulationOptions::setSize(const block::Size size) -> TerminalEmulationOptions & {
    if (size.width() < 1 || size.height() < 1 || size.width() > 10'000 || size.height() > 10'000) {
        throw err::ParameterFailure{"Terminal dimensions must be between 1 and 10,000."_el, "size"_el};
    }
    _size = size;
    return *this;
}

auto TerminalEmulationOptions::setScrollbackLimit(const block::Coordinate limit) -> TerminalEmulationOptions & {
    if (limit < 1 || limit > 10'000) {
        throw err::ParameterFailure{"Scrollback limit must be between 1 and 10,000."_el, "limit"_el};
    }
    _scrollbackLimit = limit;
    return *this;
}

auto TerminalEmulationOptions::setTabWidth(const block::Coordinate width) -> TerminalEmulationOptions & {
    if (width < 1 || width > 10'000) {
        throw err::ParameterFailure{"Tab width must be between 1 and 10,000."_el, "width"_el};
    }
    _tabWidth = width;
    return *this;
}

}
