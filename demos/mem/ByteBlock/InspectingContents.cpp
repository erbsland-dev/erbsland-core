// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Inspect and search the contents of a byte block.
///
/// Length and membership tests make common binary validation readable, while
/// `find()` and `findLast()` return byte-aware indexes for matched sequences.
void inspectingContents() {
    const auto skillPath = el::ByteBlock{el::Byte{0x10U}, el::Byte{0x21U}, el::Byte{0x34U}, el::Byte{0x21U}};

    // Validate the expected beginning, ending, and an unlock marker in the path.
    const auto hasRoot = skillPath.startsWith({el::Byte{0x10U}, el::Byte{0x21U}});
    const auto hasMastery = skillPath.endsWith({el::Byte{0x34U}, el::Byte{0x21U}});
    const auto hasUnlock = skillPath.contains({el::Byte{0x21U}});

    // Locate the first and final occurrence of the repeated unlock marker.
    const auto firstUnlock = skillPath.find({el::Byte{0x21U}});
    const auto lastUnlock = skillPath.findLast({el::Byte{0x21U}});

    el::io::printLine("Skill path         : Voie des brumes"_el);
    el::io::printLine("Byte count         : "_el, skillPath.length().toSizeT());
    el::io::printLine("Path is empty      : "_el, el::BooleanFormat::yesNo(), skillPath.isEmpty());
    el::io::printLine("Expected root      : "_el, el::BooleanFormat::yesNo(), hasRoot);
    el::io::printLine("Mastery ending     : "_el, el::BooleanFormat::yesNo(), hasMastery);
    el::io::printLine("Contains unlock    : "_el, el::BooleanFormat::yesNo(), hasUnlock);
    el::io::printLine("First unlock index : "_el, firstUnlock.toSizeT());
    el::io::printLine("Last unlock index  : "_el, lastUnlock.toSizeT());
}

}
