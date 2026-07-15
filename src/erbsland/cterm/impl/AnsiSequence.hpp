// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"

namespace erbsland::cterm::impl::ansi_sequence {

/// Build an ANSI color sequence.
[[nodiscard]] auto color(int code) -> text::String;
/// Build an ANSI foreground/background color sequence.
[[nodiscard]] auto color(int foregroundCode, int backgroundCode) -> text::String;
/// Build an absolute ANSI cursor-position sequence.
[[nodiscard]] auto cursorPosition(int row, int column) -> text::String;
/// Build an ANSI cursor-left sequence.
[[nodiscard]] auto moveLeft(int count) -> text::String;
/// Build an ANSI cursor-right sequence.
[[nodiscard]] auto moveRight(int count) -> text::String;
/// Build an ANSI cursor-up sequence.
[[nodiscard]] auto moveUp(int count) -> text::String;
/// Build an ANSI cursor-down sequence.
[[nodiscard]] auto moveDown(int count) -> text::String;

}
