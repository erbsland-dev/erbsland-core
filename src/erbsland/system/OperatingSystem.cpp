// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OperatingSystem.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"

namespace erbsland::system {

using namespace text::literals;

auto OperatingSystem::toString() const -> text::String {
    switch (_value) {
    case Windows:
        return "windows"_el;
    case Macos:
        return "macos"_el;
    case Linux:
        return "linux"_el;
    case Unknown:
    default:
        return "unknown"_el;
    }
}

}
