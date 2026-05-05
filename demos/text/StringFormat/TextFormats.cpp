// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// Text format specifications control the alignment and truncation of text output.
void textFormats() {
    const auto textPattern = el::StringFormat("{:<8}|{:>8}|{:^8}|{:.3s}"_el);
    el::io::printLine(textPattern.build("cat"_el, "cat"_el, "cat"_el, "abcdef"_el));
}
