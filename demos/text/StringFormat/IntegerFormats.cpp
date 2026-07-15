// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Integer format specifications control the base and padding of numeric output.
void integerFormats() {
    const auto pattern = el::StringFormat("{:+d} {:#x} {:.4d} {:8.4d}"_el);
    el::io::printLine(pattern.build(42, 42, 42, 42));
}

}
