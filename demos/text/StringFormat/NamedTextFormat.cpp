// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

namespace demo {

/// Named text specifications make truncation, escaping, and layout readable at the call site.
void namedTextFormat() {
    const auto heading = el::StringFormat{"{:text:maximum=12,width=18,alignment=center,fill=·}"_el};
    const auto json = el::StringFormat{"{:text:escape=json,escape-amount=non-ascii}"_el};

    el::io::printLine("|", heading.build("bodemvochtigheid"_el), "|"_el);
    el::io::printLine(json.build("sensor café\n"_el));
}

}
