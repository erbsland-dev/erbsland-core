// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The fallback to use when paragraph layout becomes impossible.
///
/// This can happen when the available width is too small for the chosen indentation, wrap markers, ellipsis marker,
/// or other paragraph settings.
enum class ParagraphOnError : uint8_t {
    /// Fallback to plain text output and let the terminal handle wrapping.
    PlainOutput,
    /// Do not output the paragraph at all.
    Empty,
};

}
