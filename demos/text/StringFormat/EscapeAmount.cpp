// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Escape format amount is controlled using a suffix after the specifier.
void escapeAmount() {
    const auto pattern = el::StringFormat("/json   : {0:/json}\n/json+  : {0:/json+}\n/json*  : {0:/json*}"_el);
    el::io::printLine(pattern.build("café\n"_el));
}

}
