// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/debug/StringDebug.hpp>

namespace demo {

constexpr auto cDebugFlags = el::DebugViewDetail::BackingStore;

/// Copies of an editor share their backing store until one editor is mutated.
/// The first write detaches that editor, preserving the other values.
///
/// From the user's perspective, every string behaves like an independent value.
/// The sharing and copying happens automatically in the background.
///
/// Constructing an editor from a read-only `String` creates editable storage.
/// Later edits can reuse that storage and its spare capacity while it remains
/// unique.
void copyOnWrite() {
    const auto source = el::String{"The treasure is hidden under the old oak tree."_el};
    auto a = el::StringEditor{source};
    auto b = a;
    auto c = b;

    const auto printAll = [&]() -> void {
        el::io::printLine("a: ", a);
        el::io::printLine(el::toDebugString(a, cDebugFlags));

        el::io::printLine("b: ", b);
        el::io::printLine(el::toDebugString(b, cDebugFlags));

        el::io::printLine("c: ", c);
        el::io::printLine(el::toDebugString(c, cDebugFlags));

        el::io::printLine();
    };

    // All three strings share the same backing store.
    el::io::printLine("After copying a -> b and b -> c"_el);
    printAll();

    // Mutating b detaches it from the shared editor storage.
    b.replaceAll("treasure"_el, "secret"_el);

    el::io::printLine("After modifying b"_el);
    printAll();

    // Mutating a detaches it too; c still preserves the original value.
    a.append(" Nobody has found it yet."_el);

    el::io::printLine("After modifying a"_el);
    printAll();
}

}
