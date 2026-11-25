// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::doc::impl {

/// A normalized capture and its human-readable command line.
struct DemoRunOutput {
    el::String text;    ///< Static ANSI capture.
    el::String command; ///< Redacted display command.
};

}
