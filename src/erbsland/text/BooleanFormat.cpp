// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BooleanFormat.hpp"

#include "Literals.hpp"

namespace erbsland::text {

auto BooleanFormat::text(const bool value) const noexcept -> StringLiteral {
    using namespace literals;

    switch (_style) {
    case Style::YesNo:
        switch (_capitalization) {
        case Capitalization::Uppercase:
            return value ? "YES"_el : "NO"_el;
        case Capitalization::Titlecase:
            return value ? "Yes"_el : "No"_el;
        default:
            return value ? "yes"_el : "no"_el;
        }
    case Style::OnOff:
        switch (_capitalization) {
        case Capitalization::Uppercase:
            return value ? "ON"_el : "OFF"_el;
        case Capitalization::Titlecase:
            return value ? "On"_el : "Off"_el;
        default:
            return value ? "on"_el : "off"_el;
        }
    case Style::EnabledDisabled:
        switch (_capitalization) {
        case Capitalization::Uppercase:
            return value ? "ENABLED"_el : "DISABLED"_el;
        case Capitalization::Titlecase:
            return value ? "Enabled"_el : "Disabled"_el;
        default:
            return value ? "enabled"_el : "disabled"_el;
        }
    default:
        switch (_capitalization) {
        case Capitalization::Uppercase:
            return value ? "TRUE"_el : "FALSE"_el;
        case Capitalization::Titlecase:
            return value ? "True"_el : "False"_el;
        default:
            return value ? "true"_el : "false"_el;
        }
    }
}

}
