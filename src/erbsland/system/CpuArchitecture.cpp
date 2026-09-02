// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CpuArchitecture.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"

namespace erbsland::system {

using namespace text::literals;

auto CpuArchitecture::toString() const -> text::String {
    switch (_value) {
    case X86:
        return "x86"_el;
    case X86_64:
        return "x86_64"_el;
    case Arm32:
        return "arm32"_el;
    case Arm64:
        return "arm64"_el;
    case Unknown:
    default:
        return "unknown"_el;
    }
}

}
