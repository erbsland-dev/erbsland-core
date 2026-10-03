// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>

namespace demo {

/// Apply an application-specific mapping to decoded characters without changing the source string.
/// Return the original character to keep it, a replacement to change it, or `Char::noCodePoint()` to omit it.
/// The callback has no captures or observable side effects and does not throw.
void customCharacterMapping() {
    const auto fieldNote = el::String{"stazione： Tir\u200Breno； boa： 07"_el};
    const auto mapCharacter = [](const el::Char character) noexcept -> el::Char {
        if (character == U'：') {
            return U':';
        }
        if (character == U'；') {
            return U',';
        }
        // Omit U+200B zero-width space.
        if (character == el::Char{0x200BU}) {
            return el::Char::noCodePoint();
        }
        return character;
    };
    const auto cleaned = fieldNote.transformed(mapCharacter);
    el::io::printLine("Field note: "_el, cleaned);

    // Assign the returned editor when this mapping is part of a mutable workflow.
    auto draft = el::StringEditor{fieldNote};
    draft = draft.transformed(mapCharacter);
    draft.append("; verificata"_el);
    el::io::printLine("Draft ....: "_el, draft);
}

}
