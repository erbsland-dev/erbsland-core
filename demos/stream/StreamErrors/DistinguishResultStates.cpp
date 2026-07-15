// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Keep timeout, end-of-stream, and failure on separate control-flow paths.
/// Timeout is a bounded result, `Finished` is a successful end state, and a stream failure throws `StreamError`.
void distinguishResultStates() {
    auto input = ScriptedByteInputStream{{7U}, 1U, 1U};
    el::io::printLine("Tiempo agotado: "_el, input.readByte().isTimeout());
    el::io::printLine("Dato marino: "_el, input.readByte().hasData());
    el::io::printLine("Fin normal: "_el, input.readByte().isFinished());
}

}
