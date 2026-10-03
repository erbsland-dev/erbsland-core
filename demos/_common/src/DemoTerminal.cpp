// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoTerminal.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>
#include <memory>

namespace demo {

using namespace el::text::literals;

DemoTerminal::DemoTerminal(const el::block::Size size, const el::cterm::TerminalFlags flags) :
    _size{size}, _flags{flags} {
    if (_flags.has(el::cterm::TerminalFlag::VirtualTty)) {
        return;
    }
    const auto environment = el::EnvironmentVariables{};
    if (!environment.get("ERBSLAND_VIRTUAL_TTY"_el).has_value()) {
        return;
    }
    _flags.set(el::cterm::TerminalFlag::VirtualTty);
    _size = {90, 40};
    if (const auto value = environment.get("ERBSLAND_VIRTUAL_TTY_SIZE"_el)) {
        const auto parts = value->splitAtFirst("x"_el);
        if (!parts) {
            throw el::ApplicationError{"ERBSLAND_VIRTUAL_TTY_SIZE must be WIDTHxHEIGHT."_el};
        }
        _size = {dimension(parts->first), dimension(parts->second)};
    }
}

auto DemoTerminal::dimension(const el::String &text) -> int {
    auto result = 0;
    if (text.isEmpty()) {
        throw el::ApplicationError{"Virtual terminal dimensions must be positive decimal integers."_el};
    }
    for (const auto character : text) {
        if (character < U'0' || character > U'9') {
            throw el::ApplicationError{"Virtual terminal dimensions must be positive decimal integers."_el};
        }
        result = std::min(2048, result * 10 + static_cast<int>(character.toRawValue() - U'0'));
    }
    if (result == 0) {
        throw el::ApplicationError{"Virtual terminal dimensions must be positive decimal integers."_el};
    }
    return result;
}

void DemoTerminal::enable(el::Application &application) {
    const auto settings = DemoTerminal{{80, 25}, {}};
    application.enableTerminal(settings._size, settings._flags);
}

auto DemoTerminal::create(const el::block::Size size, const el::cterm::TerminalFlags flags) -> el::cterm::TerminalPtr {
    const auto settings = DemoTerminal{size, flags};
    auto terminal = std::make_shared<el::cterm::Terminal>(settings._size, settings._flags);
    terminal->initializeScreen();
    return terminal;
}

}
