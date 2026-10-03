// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockAttributes.hpp"

#include "../../text/String.hpp"

namespace erbsland::cterm::impl::ansi_sequence {

/// Build the ANSI attribute transition between two fully resolved states.
/// @param previous The previous attribute state.
/// @param next The target attribute state.
/// @return SGR sequences, or an empty string when no change is needed.
/// @tested{BufferRenderTest TerminalTest}
[[nodiscard]] auto attributes(BlockAttributes previous, BlockAttributes next) -> text::String;
/// Build an ANSI color sequence.
/// @param code The code for the foreground color.
[[nodiscard]] auto color(int code) -> text::String;
/// Build an ANSI foreground/background color sequence.
/// @param foregroundCode The code for the for foreground color.
/// @param backgroundCode The code for the background color.
[[nodiscard]] auto color(int foregroundCode, int backgroundCode) -> text::String;
/// Build an absolute ANSI cursor-position sequence.
/// @param row The line number of the cursor position.
/// @param column The column number of the cursor position.
[[nodiscard]] auto cursorPosition(int row, int column) -> text::String;
/// Build an ANSI cursor-left sequence.
/// @param count The number of characters to move left.
[[nodiscard]] auto moveLeft(int count) -> text::String;
/// Build an ANSI cursor-right sequence.
/// @param count The number of characters to move right.
[[nodiscard]] auto moveRight(int count) -> text::String;
/// Build an ANSI cursor-up sequence.
/// @param count The number of lines to move up.
[[nodiscard]] auto moveUp(int count) -> text::String;
/// Build an ANSI cursor-down sequence.
/// @param count The number of lines to move down.
[[nodiscard]] auto moveDown(int count) -> text::String;

}
