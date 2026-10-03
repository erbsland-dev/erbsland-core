// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>

namespace demo {

/// `el::join()` concatenates fixed fragments and infers the string encoding.
/// `String::fromJoined()` explicitly selects the result string type.
/// `StringList::join()` combines a runtime list with an optional separator.
///
/// This is more efficient and clearer than appending in a manual loop. The
/// final size is calculated first, then the result storage is reserved once and
/// filled from the list entries.
void joiningText() {
    const auto location = "Nordljus"_el;
    const auto observation = "stjärnklart"_el;

    // Both forms concatenate the same fixed fragments without a separator.
    const auto inferredNote = el::join("14 "_el, location, ": "_el, observation);
    const auto explicitNote = el::String::fromJoined({"14 "_el, location, ": "_el, observation});
    el::io::printLine("el::join .............: "_el, inferredNote);
    el::io::printLine("String::fromJoined ...: "_el, explicitNote);

    const auto notes = el::StringList{
        inferredNote,
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
