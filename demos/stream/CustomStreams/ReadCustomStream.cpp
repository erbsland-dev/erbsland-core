// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SkillTreeStreams.hpp"

namespace demo {

/// Derive a custom byte input from `ByteInputStream` and implement one bounded `readFromSource()` operation.
/// The base class supplies exact reads, aggregate reads, integer helpers, retained partial input, and coroutine
/// wrappers.
void readCustomStream() {
    auto input = SkillTreeInputStream{{1U, 4U, 2U, 8U, 5U, 7U}};
    const auto root = input.readExact(el::ByteLength{4U});
    el::io::printLine("Skill-tree root-node bytes: "_el, root.data().length().toSizeT());
    el::io::printLine("The base class combines the underlying short reads."_el);
}

}
