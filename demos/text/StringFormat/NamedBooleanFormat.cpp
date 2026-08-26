// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

namespace demo {

/// Named Boolean specifications select the word pair, capitalization, and layout independently.
void namedBooleanFormat() {
    const auto status =
        el::StringFormat{"{:bool:style=enabled,capitalization=uppercase,width=10,alignment=right,fill=.}"_el};
    const auto answer = el::StringFormat{"{:bool:style=yes,capitalization=titlecase}"_el};

    el::io::printLine("Sensor ...: "_el, status.build(true));
    el::io::printLine("Alarm ....: "_el, answer.build(false));
}

}
