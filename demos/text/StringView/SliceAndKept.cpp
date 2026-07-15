// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `slice()` and `kept()` select the same text but make different storage
/// choices.
///
/// `slice()` returns a narrowed string that can continue to share the original
/// backing store. This is ideal for temporary parsing and for lists of views.
/// `kept()` materializes the selected range as an editable string, which lets a
/// large source string be released after the interesting part has been copied.
void sliceAndKept() {
    auto journal = el::String{"rubrik=Norrpasset|väder=klar|anteckning=Stjärnklart över sjön"_el};
    auto noteStart = journal.findLastOf(el::CharSet{U'|'});
    journal.advance(noteStart);

    // `slice()` keeps the range as a narrow view into compatible string storage.
    const auto noteSlice = journal.slice(el::ByteRange{noteStart, el::ByteLength::infinite()});

    // `kept()` copies just the selected range into an independent string.
    const auto noteCopy = journal.kept(el::ByteRange{noteStart, el::ByteLength::infinite()});

    el::io::printLine("Slice result: "_el, noteSlice);
    el::io::printLine("Kept result:  "_el, noteCopy);
    el::io::printLine("Slice byte length: "_el, noteSlice.length());
    el::io::printLine("Kept byte length:  "_el, noteCopy.length());
}

}
