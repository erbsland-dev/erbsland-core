// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `matches()` answers whether a text has the expected decoded-character shape.
///
/// A pattern without `*` matches the front of the string. A trailing `*` keeps that prefix behavior explicit.
/// A leading `*` checks the end. A pattern with text on both sides of `*` requires both the prefix and suffix without
/// overlap.
void matches() {
    const auto nameOnly = el::StringPattern{"messung:"_el};
    const auto withUnit = el::StringPattern{"messung:*;C"_el};
    const auto finished = el::StringPattern{"*;fertig"_el};

    const auto temperature = el::StringView{"messung:temperatur=21.4;C"_el};
    const auto completed = el::StringView{"messung:leitwert=0.42;fertig"_el};

    el::io::printLine("front only ..............: "_el, nameOnly.matches(temperature));
    el::io::printLine("front and back ..........: "_el, withUnit.matches(temperature));
    el::io::printLine("suffix only .............: "_el, finished.matches(completed));
    el::io::printLine("wrong suffix ............: "_el, finished.matches(temperature));
    el::io::printLine("missing suffix ..........: "_el, withUnit.matches("messung:temperatur=21.4"_el));
}

}
