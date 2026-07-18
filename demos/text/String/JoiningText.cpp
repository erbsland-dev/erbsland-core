// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `join()` combines all entries from a string list with an optional separator.
///
/// This is more efficient and clearer than appending in a manual loop. The
/// final size is calculated first, then the result storage is reserved once and
/// filled from the list entries.
void joiningText() {
    const auto notes = el::StringList{
        "14 Nordljus: stjärnklart"_el,
        "12 Åsleden: klar sikt"_el,
        "13 Norrpasset: dimma"_el,
    };

    // Lists can be transformed first, then joined into the final document.
    const auto sortedNotes = notes.sorted(el::Char::compareCaseFolded);
    const auto document = sortedNotes.join("\n"_el);

    el::io::printLine("Sorted journal page:"_el);
    el::io::printLine(document);
}

}
