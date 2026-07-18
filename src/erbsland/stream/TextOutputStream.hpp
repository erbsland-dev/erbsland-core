// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OutputStream.hpp"
#include "TextOutputStream_fwd.hpp"
#include "TextPrintContext.hpp"

#include "../math/AnyIntegerTypes.hpp"
#include "../text/AnyStringBuilder.hpp"
#include "../text/Char.hpp"
#include "../text/FloatFormat.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringEncoding.hpp"
#include "../util/CoTask.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::stream {

/// A stream that writes decoded Unicode text.
/// Text output streams write characters and read-only strings. `writeLine` appends a single line-feed character after
/// the optional text. Every call is atomic: `Timeout` means none of its encoded output was accepted and the complete
/// call can be retried. Encoding invalid text uses replacement behavior.
/// @tested{EncodedTextStreamTest StandardTextOutputStreamTest AnyStringBuilderStreamTest AsyncStreamTest}
class TextOutputStream : public OutputStream {
public:
    ~TextOutputStream() override = default;

public: // accessors
    /// Get the encoding configured for the stream.
    [[nodiscard]] virtual auto encoding() const noexcept -> text::StringEncoding = 0;
    /// Get the effective encoding used by the stream.
    [[nodiscard]] virtual auto effectiveEncoding() const noexcept -> text::StringEncoding = 0;

public: // core interface
    /// Write one character.
    /// @param character The character to write.
    /// @return `Success` if the character was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto write(text::Char character) -> StreamWriteStatus = 0;
    /// Write text.
    /// @param text The text to write.
    /// @return `Success` if all text was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto write(const text::String &text) -> StreamWriteStatus = 0;
    /// Write a line-feed character.
    /// @return `Success` if the line feed was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto writeLine() -> StreamWriteStatus = 0;
    /// Write text followed by a line-feed character.
    /// @param text The text to write before the line-feed.
    /// @return `Success` if the complete line was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto writeLine(const text::String &text) -> StreamWriteStatus = 0;

public: // coroutine interface
    /// Asynchronously write owned text.
    /// @param text The text retained by the operation until it completes.
    /// @return A task with the same result as `write(String)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing target fails.
    [[nodiscard]] auto coWrite(text::String text) -> util::CoTask<StreamWriteStatus>;
    /// Asynchronously write one line-feed character.
    /// @return A task with the same result as `writeLine()`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing target fails.
    [[nodiscard]] auto coWriteLine() -> util::CoTask<StreamWriteStatus>;
    /// Asynchronously write owned text followed by a line-feed character.
    /// @param text The text retained by the operation until it completes.
    /// @return A task with the same result as `writeLine(String)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing target fails.
    [[nodiscard]] auto coWriteLine(text::String text) -> util::CoTask<StreamWriteStatus>;

public: // convenience print interface
    /// Print one or more arguments to the stream.
    /// This is a convenience helper for small output.
    /// For large text, prefer the low-level `write...` methods.
    /// Valid arguments are chars, string types, integers, floating-point values, booleans, values with
    /// `toString() const` returning `text::String`, and values with `toRawValue() const`
    /// returning a supported non-character integer. If both conversion methods exist, `toString()` is used.
    /// Change the integer or floating-point format by adding a format specifier before the affected value.
    /// @param args The arguments to print.
    /// @return `Success` if the complete formatted output was accepted, or `Timeout` if nothing was accepted.
    template <typename... tArgs>
    auto print(const tArgs &...args) -> StreamWriteStatus {
        const auto context = createPrintContext();
        (context->print(args), ...);
        return context->commit();
    }
    /// Print one or more arguments to the stream and add a line-break.
    /// @see print() for all details.
    /// @param args The arguments to print.
    /// @return `Success` if the complete formatted line was accepted, or `Timeout` if nothing was accepted.
    template <typename... tArgs>
    auto printLine(const tArgs &...args) -> StreamWriteStatus {
        const auto context = createPrintContext();
        (context->print(args), ...);
        context->print(U'\n');
        return context->commit();
    }

protected: // print internals
    /// Create a new temporary print context that only lives for the print method.
    /// The print methods will call any number of `print()` methods.
    /// At the end, it will call `commit()` that shall write everything to this stream.
    /// An implementation is allowed to handle things differently if the result is the same.
    virtual auto createPrintContext() -> TextPrintContextPtr;
};

}
