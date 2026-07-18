// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/debug/StringDebug.hpp>

namespace demo {

constexpr auto cDebugFlags = el::DebugViewDetail::BackingStore;

/// Copy-on-write allows strings to be copied at almost no cost.
/// Multiple string objects can share the same backing store until one of them
/// is modified. At that point, only the modified string receives its own copy
/// of the data.
///
/// From the user's perspective, every string behaves like an independent value.
/// The sharing and copying happens automatically in the background.
///
/// This demo visualizes how the backing store changes as strings are copied and
/// modified.
void copyOnWrite() {

    // Create the original string and two copies.
    auto a = el::StringEditor{"The treasure is hidden under the old oak tree."_el};
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

    // Modifying b causes it to detach from the shared data.
    b.replaceAll("treasure"_el, "secret"_el);

    el::io::printLine("After modifying b"_el);
    printAll();

    // a and c still share the original backing store.
    // Modifying a creates another independent copy.
    a.append(" Nobody has found it yet."_el);

    el::io::printLine("After modifying a"_el);
    printAll();
}

}
