// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Escape format specifications control the HTML escaping of text output.
void escapeFormat() {
    const auto pattern = el::StringFormat("<p>{:/html}</p>\n{:>12/html}"_el);
    el::io::printLine(pattern.build("<script>alert(\"xss\")</script>"_el, "<p>"_el));
}

}
