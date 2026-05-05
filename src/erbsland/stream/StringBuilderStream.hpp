// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextOutputStream.hpp"

#include "../text/StringBuilder.hpp"

namespace erbsland::stream {

class StringBuilderStream;
using StringBuilderStreamPtr = std::shared_ptr<StringBuilderStream>;

/// A stream to build strings.
/// @tested{StringBuilderStreamTest}
class StringBuilderStream : public TextOutputStream {
public:
    /// Create a new string builder stream that uses the given string kind for the output.
    /// @param stringKind The kind of string to use for the output.
    explicit StringBuilderStream(text::StringKind stringKind = text::StringKind::U8);

    // defaults
    ~StringBuilderStream() override = default;
    StringBuilderStream(const StringBuilderStream &) = default;
    StringBuilderStream(StringBuilderStream &&) = default;
    auto operator=(const StringBuilderStream &) -> StringBuilderStream & = default;
    auto operator=(StringBuilderStream &&) -> StringBuilderStream & = default;

public: // factory
    /// Create a new string builder stream with the given string kind as shared pointer.
    /// @param stringKind The kind of string to use for the output.
    [[nodiscard]] static auto create(text::StringKind stringKind = text::StringKind::U8) -> StringBuilderStreamPtr {
        return std::make_shared<StringBuilderStream>(stringKind);
    }

public: // StringBuilder methods
    /// Get the target string kind.
    [[nodiscard]] auto kind() const noexcept -> text::StringKind;
    /// Get the current decoded code-point length.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;
    /// Test if this builder is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Clear all built text while keeping the target kind.
    void clear() noexcept;
    /// Create a UTF-8 string copy.
    [[nodiscard]] auto toU8String() const -> text::U8String;
    /// @overload
    [[nodiscard]] auto toString() const -> text::U8String;
    /// Create a UTF-16 string copy.
    [[nodiscard]] auto toU16String() const -> text::U16String;
    /// Create a UTF-32 string copy.
    [[nodiscard]] auto toU32String() const -> text::U32String;
    /// Create an "any" string copy.
    [[nodiscard]] auto toAnyString() const -> text::AnyString;
    /// Move out a UTF-8 string and reset this builder.
    [[nodiscard]] auto takeU8String() -> text::U8String;
    /// @overload
    [[nodiscard]] auto takeString() -> text::String;
    /// Move out a UTF-16 string and reset this builder.
    [[nodiscard]] auto takeU16String() -> text::U16String;
    /// Move out a UTF-32 string and reset this builder.
    [[nodiscard]] auto takeU32String() -> text::U32String;
    /// Move out "any" string and reset this builder.
    [[nodiscard]] auto takeAnyString() -> text::AnyString;

public: // implement TextOutputStream
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void flush() override;
    void close() override;
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    void write(text::Char character) override;
    void write(const text::StringView &text) override;
    void writeLine() override;
    void writeLine(const text::StringView &text) override;

protected: // override TextOutputStream
    auto createPrintContext() -> TextPrintContextPtr override;

private:
    text::StringBuilder _builder;
};

}
