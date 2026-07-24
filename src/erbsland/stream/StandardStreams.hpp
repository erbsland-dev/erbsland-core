// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SensitiveInput.hpp"
#include "StandardStreamRedirect.hpp"
#include "TextInputStream.hpp"
#include "TextOutputStream.hpp"

namespace erbsland::stream {

/// Get the process standard input stream.
/// The returned stream is cached and shared by the whole process. Calling `close()` on this stream has no effect.
/// @return The standard input stream.
/// @throws stream::StreamError If the process standard input stream is not available.
[[nodiscard]] auto stdIn() -> TextInputStreamPtr;
/// Get the process standard output stream.
/// The returned stream is cached and shared by the whole process. Calling `close()` on this stream has no effect.
/// @return The standard output stream.
/// @throws stream::StreamError If the process standard output stream is not available.
[[nodiscard]] auto stdOut() -> TextOutputStreamPtr;
/// Get the process standard error stream.
/// The returned stream is cached and shared by the whole process. Calling `close()` on this stream has no effect.
/// @return The standard error stream.
/// @throws stream::StreamError If the process standard error stream is not available.
[[nodiscard]] auto stdErr() -> TextOutputStreamPtr;
/// Replace the process standard input stream for the lifetime of the returned guard.
/// Existing pointers returned by `stdIn()` keep using the active replacement.
/// @param input The replacement input stream.
/// @return The guard that restores the previous input stream target.
/// @throws stream::StreamError If `input` is empty.
[[nodiscard]] auto redirectStdIn(TextInputStreamPtr input) -> StandardStreamRedirect;
/// Replace the process standard output stream for the lifetime of the returned guard.
/// Existing pointers returned by `stdOut()` keep using the active replacement.
/// @param output The replacement output stream.
/// @return The guard that restores the previous output stream target.
/// @throws stream::StreamError If `output` is empty.
[[nodiscard]] auto redirectStdOut(TextOutputStreamPtr output) -> StandardStreamRedirect;
/// Replace the process standard error stream for the lifetime of the returned guard.
/// Existing pointers returned by `stdErr()` keep using the active replacement.
/// @param error The replacement error stream.
/// @return The guard that restores the previous error stream target.
/// @throws stream::StreamError If `error` is empty.
[[nodiscard]] auto redirectStdErr(TextOutputStreamPtr error) -> StandardStreamRedirect;
/// Replace both process standard text streams for the lifetime of the returned guard.
/// Existing pointers returned by `stdOut()` and `stdErr()` keep using the active replacements.
/// @param output The replacement output stream.
/// @param error The replacement error stream.
/// @return The guard that restores both previous stream targets.
/// @throws stream::StreamError If a replacement stream is empty.
[[nodiscard]] auto redirectStandardStreams(TextOutputStreamPtr output, TextOutputStreamPtr error)
    -> StandardStreamRedirect;

namespace io {

/// @copydoc TextOutputStream::write(text::Char)
inline auto write(text::Char character) -> StreamWriteStatus {
    return stdOut()->write(character);
}
/// @copydoc TextOutputStream::write(const text::String&)
inline auto write(const text::String &text) -> StreamWriteStatus {
    return stdOut()->write(text);
}
/// @copydoc TextOutputStream::writeLine()
inline auto writeLine() -> StreamWriteStatus {
    return stdOut()->writeLine();
}
/// @copydoc TextOutputStream::writeLine(const text::String&)
inline auto writeLine(const text::String &text) -> StreamWriteStatus {
    return stdOut()->writeLine(text);
}
/// @copydoc TextOutputStream::print()
template <typename... tArgs>
auto print(const tArgs &...args) -> StreamWriteStatus {
    return stdOut()->print(args...);
}
/// @copydoc TextOutputStream::printLine()
template <typename... tArgs>
auto printLine(const tArgs &...args) -> StreamWriteStatus {
    return stdOut()->printLine(args...);
}
/// @copydoc TextOutputStream::print()
template <typename... tArgs>
auto printError(const tArgs &...args) -> StreamWriteStatus {
    return stdErr()->print(args...);
}
/// @copydoc TextOutputStream::printLine()
template <typename... tArgs>
auto printErrorLine(const tArgs &...args) -> StreamWriteStatus {
    return stdErr()->printLine(args...);
}

}

}
