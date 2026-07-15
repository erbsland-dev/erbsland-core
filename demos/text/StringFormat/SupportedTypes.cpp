// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// The formatter accepts text, integers, floats, booleans, and characters.
void supportedTypes() {
    const auto pattern = el::StringFormat("{} {} {} {} {}"_el);
    el::io::printLine(pattern.build(42, 3.14, true, 'A', "hello"_el));
}

}
