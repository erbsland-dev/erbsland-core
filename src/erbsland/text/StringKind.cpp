// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringKind.hpp"

#include "Literals.hpp"
#include "String.hpp"

#include <array>

namespace erbsland::text {

auto toString(StringKind kind) -> String {
    using namespace literals;
    static auto const names = std::array<String, 3>{
        String{"U8"_el},
        String{"U16"_el},
        String{"U32"_el},
    };
    return names[std::min(static_cast<std::size_t>(kind), names.size() - 1U)];
}

}
