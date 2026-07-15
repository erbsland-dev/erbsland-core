// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// A positioning timeout leaves the logical position unchanged.
/// Retry the complete positioning request after the stream becomes ready instead of guessing how far it moved.
void handlePositionTimeout() {
    auto output = ScriptedByteOutputStream{};
    output.enablePositioning(1U);
    const auto before = output.position();
    const auto first = output.setPosition(el::ByteIndex{12U});
    el::io::printLine("First positioning attempt timed out: "_el, first.isTimeout());
    el::io::printLine("Position is unchanged: "_el, output.position() == before);
    output.setPosition(el::ByteIndex{12U});
    el::io::printLine("Position after retry: "_el, output.position().toSizeT());
}

}
