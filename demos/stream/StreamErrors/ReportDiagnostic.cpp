// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Convert a stream exception into the common diagnostic document used by logs and user interfaces.
/// This avoids constructing a second message and preserves the original stream-domain context.
void reportDiagnostic() {
    auto output = ScriptedByteOutputStream{};
    output.setFailure(true);
    try {
        output.flush();
    } catch (const el::StreamError &error) {
        const auto document = el::DiagnosticHelper{error}.toDocument();
        el::io::printLine(document.toString());
    }
}

}
