// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `shrinkToFit()` is a deliberate compaction step, not routine cleanup.
///
/// A small construction can deliberately reserve its final maximum size.
/// Calling `shrinkToFit()` after construction releases unused capacity before
/// the editor is converted to a stored read-only string.
void shrinkSlices() {
    auto note = el::StringEditor{};
    note.reserve(el::ByteLength{96U});
    note.append("Specimen: "_el);
    note.append("Luzula sylvatica"_el);

    el::io::printLine("Before compaction:"_el);
    el::io::printLine("  text ........: "_el, note);
    el::io::printLine("  length ......: "_el, note.length());
    el::io::printLine("  capacity ....: "_el, note.capacity());
    el::io::printLine("  memory usage : "_el, note.memoryUsage());

    note.shrinkToFit();
    el::io::printLine();
    el::io::printLine("After shrinkToFit:"_el);
    el::io::printLine("  text ........: "_el, note);
    el::io::printLine("  length ......: "_el, note.length());
    el::io::printLine("  capacity ....: "_el, note.capacity());
    el::io::printLine("  memory usage : "_el, note.memoryUsage());

    const auto storedNote = el::String{note};
    el::io::printLine("Stored result .: "_el, storedNote);
}

}
