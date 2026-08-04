// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// `StreamError` carries a structured, user-facing context.
/// Handlers can inspect its title, description, recovery help, path, and optional native platform context.
void inspectStreamError() {
    auto input = ScriptedByteInputStream{{1U}};
    input.setFailure(true);
    try {
        [[maybe_unused]] const auto byte = input.readByte();
        // ... process the byte ...
    } catch (const el::StreamError &error) {
        el::io::printLine("Title: "_el, error.title());
        el::io::printLine("Description: "_el, error.description());
        el::io::printLine("Estado Failed: "_el, input.state() == el::StreamState::Failed);
    }
}

}
