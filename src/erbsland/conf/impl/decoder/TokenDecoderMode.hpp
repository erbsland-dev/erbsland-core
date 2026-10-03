// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf::impl {

/// Select strict configuration parsing or recovering syntax recognition.
enum class TokenDecoderMode {
    Parsing, ///< Propagate input errors and allow configured placeholder expansion.
    Syntax,  ///< Retain input diagnostics without expanding placeholders.
};

}
