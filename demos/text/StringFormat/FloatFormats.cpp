// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Float format specifications control the precision and sign of floating-point output.
void floatFormats() {
    const auto floatPattern = el::StringFormat("{:.2f} {:+8.1f}"_el);
    el::io::printLine(floatPattern.build(12.345, 1.25));
}

}
