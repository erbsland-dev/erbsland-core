// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// An empty placeholder accepts any supported type and formats it using the default format.
void emptyPlaceholder() {
    const auto pattern = el::StringFormat("a: {} b: {} c: {} d: {}");
    el::io::printLine(pattern.build(123, false, 76.92, "text"_el));
}
