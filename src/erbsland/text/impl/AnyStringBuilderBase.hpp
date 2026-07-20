// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringAppendTools.hpp"

#include "../AnyStringEditor_fwd.hpp"
#include "../ByteFormat_fwd.hpp"
#include "../Char.hpp"
#include "../StringKind.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringEditor_fwd.hpp"
#include "../u16/U16StringLiteral_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u32/U32StringLiteral_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"
#include "../u8/U8StringLiteral_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/SharedVirtualData.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/ElementCount.hpp"

namespace erbsland::text::impl {

/// Abstract base class for string builder backends.
/// @tested{AnyStringBuilderTest}
class AnyStringBuilderBase : public mem::SharedVirtualData, public StringAppendTools {
public:
    AnyStringBuilderBase() = default;
    AnyStringBuilderBase(const AnyStringBuilderBase &) = default;
    AnyStringBuilderBase(AnyStringBuilderBase &&) = default;
    auto operator=(const AnyStringBuilderBase &) -> AnyStringBuilderBase & = default;
    auto operator=(AnyStringBuilderBase &&) -> AnyStringBuilderBase & = default;
    ~AnyStringBuilderBase() override = default;

public:
    /// Create an unreferenced polymorphic copy.
    [[nodiscard]] auto clone() const -> AnyStringBuilderBase * override = 0;
    /// Get the target string kind.
    [[nodiscard]] virtual auto kind() const noexcept -> StringKind = 0;
    /// Get the current decoded code-point length.
    [[nodiscard]] virtual auto length() const noexcept -> unit::CpLength = 0;
    /// Test if the builder is empty.
    [[nodiscard]] virtual auto isEmpty() const noexcept -> bool = 0;
    /// Clear the builder.
    virtual void clear() noexcept = 0;
    /// Append one Unicode code point.
    /// @return The number of code points appended.
    auto append(Char character) -> unit::CpLength override = 0;
    /// Append one Unicode code point multiple times.
    virtual void append(Char character, unit::CpLength count) = 0;
    /// Append a UTF-8 read-only string.
    /// @return The number of code points appended.
    auto append(const U8String &text) -> unit::CpLength override = 0;
    /// Append a UTF-8 read-only string multiple times.
    virtual void append(const U8String &text, unit::ElementCount count) = 0;
    /// Append a UTF-16 read-only string.
    /// @return The number of code points appended.
    auto append(const U16String &text) -> unit::CpLength override = 0;
    /// Append a UTF-16 read-only string multiple times.
    virtual void append(const U16String &text, unit::ElementCount count) = 0;
    /// Append a UTF-32 read-only string.
    /// @return The number of code points appended.
    auto append(const U32String &text) -> unit::CpLength override = 0;
    /// Append a UTF-32 read-only string multiple times.
    virtual void append(const U32String &text, unit::ElementCount count) = 0;
    /// Append a byte block as formatted hexadecimal text.
    virtual void appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) = 0;
    /// Create a UTF-8 string copy.
    [[nodiscard]] virtual auto toU8StringEditor() const -> U8StringEditor = 0;
    /// Create a UTF-16 string copy.
    [[nodiscard]] virtual auto toU16StringEditor() const -> U16StringEditor = 0;
    /// Create a UTF-32 string copy.
    [[nodiscard]] virtual auto toU32StringEditor() const -> U32StringEditor = 0;
    /// Create an "any" string copy.
    [[nodiscard]] virtual auto toAnyStringEditor() const -> AnyStringEditor = 0;
    /// Move out a UTF-8 string and reset this builder.
    [[nodiscard]] virtual auto takeU8StringEditor() -> U8StringEditor = 0;
    /// Move out a UTF-16 string and reset this builder.
    [[nodiscard]] virtual auto takeU16StringEditor() -> U16StringEditor = 0;
    /// Move out a UTF-32 string and reset this builder.
    [[nodiscard]] virtual auto takeU32StringEditor() -> U32StringEditor = 0;
    /// Move out an "any" string and reset this builder.
    [[nodiscard]] virtual auto takeAnyStringEditor() -> AnyStringEditor = 0;
};

}
