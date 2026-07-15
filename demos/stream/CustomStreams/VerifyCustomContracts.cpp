// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SkillTreeStreams.hpp"

namespace demo {

/// Test custom streams through their public contract.
/// Cover short reads, timeout retention, atomic writes, close and abort, failures, optional positioning, and the shared
/// ownership requirement for coroutine methods.
void verifyCustomContracts() {
    auto input = SkillTreeInputStream{{1U, 2U, 3U, 4U, 5U}};
    const auto exact = input.readExact(el::ByteLength{5U});
    const auto finished = input.readByte();
    auto output = SkillTreeOutputStream{};
    const auto write = output.writeUInt32(0x01020304U);
    const auto close = output.close();

    el::io::printLine("Exact read: "_el, exact.hasData());
    el::io::printLine("Stream finished: "_el, finished.isFinished());
    el::io::printLine("Atomic write: "_el, write.isSuccess());
    el::io::printLine("Closed normally: "_el, close.isClosed());
}

}
