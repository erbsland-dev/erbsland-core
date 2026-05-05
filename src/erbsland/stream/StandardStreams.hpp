// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextOutputStream.hpp"

namespace erbsland::stream {

/// Get the process standard output stream.
/// The returned stream is cached and shared by the whole process. Calling `close()` on this stream has no effect.
/// @return The standard output stream.
/// @throws err::StreamError If the process standard output stream is not available.
/// @tested{StandardStreamsTest}
[[nodiscard]] auto stdOut() -> TextOutputStreamPtr;
/// Get the process standard error stream.
/// The returned stream is cached and shared by the whole process. Calling `close()` on this stream has no effect.
/// @return The standard error stream.
/// @throws err::StreamError If the process standard error stream is not available.
/// @tested{StandardStreamsTest}
[[nodiscard]] auto stdErr() -> TextOutputStreamPtr;

namespace io {

/// @copydoc TextOutputStream::write(text::Char)
inline void write(text::Char character) {
    stdOut()->write(character);
}
/// @copydoc TextOutputStream::write(const text::StringView&)
inline void write(const text::StringView &text) {
    stdOut()->write(text);
}
/// @copydoc TextOutputStream::writeLine()
inline void writeLine() {
    stdOut()->writeLine();
}
/// @copydoc TextOutputStream::writeLine(const text::StringView&)
inline void writeLine(const text::StringView &text) {
    stdOut()->writeLine(text);
}
/// @copydoc TextOutputStream::print()
template <typename... tArgs>
void print(const tArgs &...args) {
    stdOut()->print(args...);
}
/// @copydoc TextOutputStream::printLine()
template <typename... tArgs>
void printLine(const tArgs &...args) {
    stdOut()->printLine(args...);
}
/// @copydoc TextOutputStream::print()
template <typename... tArgs>
void printError(const tArgs &...args) {
    stdErr()->print(args...);
}
/// @copydoc TextOutputStream::printLine()
template <typename... tArgs>
void printErrorLine(const tArgs &...args) {
    stdErr()->printLine(args...);
}

}

}
