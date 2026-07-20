// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringKind.hpp"

#include "Literals.hpp"
#include "String.hpp"

#include <array>

namespace erbsland::text {

using namespace literals;

auto toString(StringKind kind) -> String {
    static auto const names = std::array<String, 3>{
        "U8"_el,
        "U16"_el,
        "U32"_el,
    };
    return names[std::min(static_cast<std::size_t>(kind), names.size() - 1U)];
}

}
