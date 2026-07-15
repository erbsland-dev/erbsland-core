// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Aborting a temporary stream returns immediately and schedules removal when automatic cleanup is enabled.
/// Destruction uses the same fallback, which keeps exception paths and shutdown from waiting on native I/O.
void cleanupAfterAbort() {
    const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
    output->writeUInt32(0x4d414d4dU);
    el::io::printLine("Automatic cleanup: "_el, output->removeOnClose());
    output->abort();
    el::io::printLine("Abort returned with a closed stream: "_el, output->state() == el::StreamState::Closed);
}

}
