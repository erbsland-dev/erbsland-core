// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Positional placeholders allow you to specify the order of the arguments in the format string.
void positionalPlaceholders() {
    const auto pattern = el::StringFormat("<{1}>{0}</{1}>");
    el::io::printLine(pattern.build("text"_el, "h1"_el));
}

}
