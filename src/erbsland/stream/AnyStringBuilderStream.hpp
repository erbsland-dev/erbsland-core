// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextOutputStream.hpp"

#include "../text/AnyStringBuilder.hpp"

#include <mutex>

namespace erbsland::stream {

class AnyStringBuilderStream;
using AnyStringBuilderStreamPtr = std::shared_ptr<AnyStringBuilderStream>;

/// A stream to build strings.
/// @tested{AnyStringBuilderStreamTest}
class AnyStringBuilderStream : public TextOutputStream {
private:
    class ConstructionToken final {
        friend class AnyStringBuilderStream;

        ConstructionToken() = default;
    };

public:
    /// Internal constructor used by `create()`.
    /// @param stringKind The kind of string to use for the output.
    /// @param token The private factory token.
    explicit AnyStringBuilderStream(text::StringKind stringKind, ConstructionToken token);

    // defaults
    ~AnyStringBuilderStream() override { abort(); }
    AnyStringBuilderStream(const AnyStringBuilderStream &) = delete;
    AnyStringBuilderStream(AnyStringBuilderStream &&) = delete;
    auto operator=(const AnyStringBuilderStream &) -> AnyStringBuilderStream & = delete;
    auto operator=(AnyStringBuilderStream &&) -> AnyStringBuilderStream & = delete;

public: // factory
    /// Create a new string builder stream with the given string kind as shared pointer.
    /// @param stringKind The kind of string to use for the output.
    /// @return A shared-owned string builder stream.
    [[nodiscard]] static auto create(text::StringKind stringKind = text::StringKind::U8) -> AnyStringBuilderStreamPtr {
        return std::make_shared<AnyStringBuilderStream>(stringKind, ConstructionToken{});
    }

public: // AnyStringBuilder methods
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
    /// Create an editable UTF-8 string copy.
    [[nodiscard]] auto toU8StringEditor() const -> text::U8StringEditor;
    /// @overload
    [[nodiscard]] auto toStringEditor() const -> text::StringEditor;
    /// Create an editable UTF-16 string copy.
    [[nodiscard]] auto toU16StringEditor() const -> text::U16StringEditor;
    /// Create an editable UTF-32 string copy.
    [[nodiscard]] auto toU32StringEditor() const -> text::U32StringEditor;
    /// Create an editable type-erased string copy.
    [[nodiscard]] auto toAnyStringEditor() const -> text::AnyStringEditor;
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
    /// Move out an editable UTF-8 string and reset this builder.
    [[nodiscard]] auto takeU8StringEditor() -> text::U8StringEditor;
    /// @overload
    [[nodiscard]] auto takeStringEditor() -> text::StringEditor;
    /// Move out an editable UTF-16 string and reset this builder.
    [[nodiscard]] auto takeU16StringEditor() -> text::U16StringEditor;
    /// Move out an editable UTF-32 string and reset this builder.
    [[nodiscard]] auto takeU32StringEditor() -> text::U32StringEditor;
    /// Move out an editable type-erased string and reset this builder.
    [[nodiscard]] auto takeAnyStringEditor() -> text::AnyStringEditor;

public: // implement TextOutputStream
    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override { return _settings; }
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    auto write(text::Char character) -> StreamWriteStatus override;
    auto write(const text::String &text) -> StreamWriteStatus override;
    auto writeLine() -> StreamWriteStatus override;
    auto writeLine(const text::String &text) -> StreamWriteStatus override;

private:
    mutable std::mutex _mutex;
    text::AnyStringBuilder _builder;
    OutputStreamSettings _settings;
    StreamState _state{StreamState::Open};
};

}
