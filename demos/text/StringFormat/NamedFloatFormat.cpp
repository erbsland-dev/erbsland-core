// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

namespace demo {

/// Named number specifications use notation to select floating-point formatting.
void namedFloatFormat() {
    const auto fixed = el::StringFormat{"{:number:notation=fixed,precision=2,sign=always}"_el};
    const auto scientific = el::StringFormat{"{:number:notation=scientific,precision=3,letter-case=uppercase}"_el};

    el::io::printLine("Temperature ...: "_el, fixed.build(21.375));
    el::io::printLine("Pressure ......: "_el, scientific.build(1013.25));
}

}
