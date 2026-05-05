// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OutputStream.hpp"
#include "TextPrintContext.hpp"

#include "../math/AnyIntegerTypes.hpp"
#include "../text/Char.hpp"
#include "../text/FloatFormat.hpp"
#include "../text/String.hpp"
#include "../text/StringBuilder.hpp"
#include "../text/StringEncoding.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::stream {

class TextOutputStream;
using TextOutputStreamPtr = std::shared_ptr<TextOutputStream>;

/// A stream that writes decoded Unicode text.
/// Text output streams write characters and string views. `writeLine` appends a single line-feed character after the
/// optional text. Encoding invalid text uses replacement behavior.
/// @tested{EncodedTextStreamTest StandardTextOutputStreamTest}
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
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void write(text::Char character) = 0;
    /// Write text.
    /// @param text The text to write.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void write(const text::StringView &text) = 0;
    /// Write a line-feed character.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void writeLine() = 0;
    /// Write text followed by a line-feed character.
    /// @param text The text to write before the line-feed.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void writeLine(const text::StringView &text) = 0;

public: // convenience print interface
    /// Print one or more arguments to the stream.
    /// This is a convenience helper for small output.
    /// For large text, prefer the low-level `write...` methods.
    /// Valid arguments are chars, string types, integers, floating-point values, booleans, values with
    /// `toString() const` returning `text::String` or `text::StringView`, and values with `toRawValue() const`
    /// returning a supported non-character integer. If both conversion methods exist, `toString()` is used.
    /// Change the integer or floating-point format by adding a format specifier before the affected value.
    /// @param args The arguments to print.
    template <typename... tArgs>
    void print(const tArgs &...args) {
        const auto context = createPrintContext();
        (context->print(args), ...);
        context->commit();
    }
    /// Print one or more arguments to the stream and add a line-break.
    /// @see print() for all details.
    /// @param args The arguments to print.
    template <typename... tArgs>
    void printLine(const tArgs &...args) {
        const auto context = createPrintContext();
        (context->print(args), ...);
        context->print(U'\n');
        context->commit();
    }

protected: // print internals
    /// Create a new temporary print context that only lives for the print method.
    /// The print methods will call any number of `print()` methods.
    /// At the end, it will call `commit()` that shall write everything to this stream.
    /// An implementation is allowed to handle things differently if the result is the same.
    virtual auto createPrintContext() -> TextPrintContextPtr;
};

}
