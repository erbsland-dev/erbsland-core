// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionType.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"

namespace erbsland::options {

using namespace text::literals;

auto OptionType::toString() const -> text::String {
    switch (_type) {
    case Flag:
        return "flag"_el;
    case Boolean:
        return "boolean"_el;
    case Integer:
        return "integer"_el;
    case Text:
        return "text"_el;
    case SensitiveText:
        return "sensitive-text"_el;
    case Choice:
        return "choice"_el;
    }
    return "unknown"_el;
}

}
