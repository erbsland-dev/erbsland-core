// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../AnyString_fwd.hpp"
#include "../Char.hpp"
#include "../StringKind.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringLiteral_fwd.hpp"
#include "../u16/U16StringView_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringLiteral_fwd.hpp"
#include "../u32/U32StringView_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringLiteral_fwd.hpp"
#include "../u8/U8StringView_fwd.hpp"

#include "../../mem/SharedVirtualData.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/ElementCount.hpp"

namespace erbsland::text::impl {

/// Abstract base class for string builder backends.
/// @tested{StringBuilderTest}
class StringBuilderBase : public mem::SharedVirtualData {
public:
    StringBuilderBase() = default;
    StringBuilderBase(const StringBuilderBase &) = default;
    StringBuilderBase(StringBuilderBase &&) = default;
    auto operator=(const StringBuilderBase &) -> StringBuilderBase & = default;
    auto operator=(StringBuilderBase &&) -> StringBuilderBase & = default;
    ~StringBuilderBase() override = default;

public:
    /// Create an unreferenced polymorphic copy.
    [[nodiscard]] auto clone() const -> StringBuilderBase * override = 0;
    /// Get the target string kind.
    [[nodiscard]] virtual auto kind() const noexcept -> StringKind = 0;
    /// Get the current decoded code-point length.
    [[nodiscard]] virtual auto length() const noexcept -> unit::CpLength = 0;
    /// Test if the builder is empty.
    [[nodiscard]] virtual auto isEmpty() const noexcept -> bool = 0;
    /// Clear the builder.
    virtual void clear() noexcept = 0;
    /// Append one Unicode code point.
    virtual void append(Char character) = 0;
    /// Append one Unicode code point multiple times.
    virtual void append(Char character, unit::CpLength count) = 0;
    /// Append a UTF-8 string view.
    virtual void append(const U8StringView &text) = 0;
    /// Append a UTF-8 string view multiple times.
    virtual void append(const U8StringView &text, unit::ElementCount count) = 0;
    /// Append a UTF-16 string view.
    virtual void append(const U16StringView &text) = 0;
    /// Append a UTF-16 string view multiple times.
    virtual void append(const U16StringView &text, unit::ElementCount count) = 0;
    /// Append a UTF-32 string view.
    virtual void append(const U32StringView &text) = 0;
    /// Append a UTF-32 string view multiple times.
    virtual void append(const U32StringView &text, unit::ElementCount count) = 0;
    /// Create a UTF-8 string copy.
    [[nodiscard]] virtual auto toU8String() const -> U8String = 0;
    /// Create a UTF-16 string copy.
    [[nodiscard]] virtual auto toU16String() const -> U16String = 0;
    /// Create a UTF-32 string copy.
    [[nodiscard]] virtual auto toU32String() const -> U32String = 0;
    /// Create an "any" string copy.
    [[nodiscard]] virtual auto toAnyString() const -> AnyString = 0;
    /// Move out a UTF-8 string and reset this builder.
    [[nodiscard]] virtual auto takeU8String() -> U8String = 0;
    /// Move out a UTF-16 string and reset this builder.
    [[nodiscard]] virtual auto takeU16String() -> U16String = 0;
    /// Move out a UTF-32 string and reset this builder.
    [[nodiscard]] virtual auto takeU32String() -> U32String = 0;
    /// Move out an "any" string and reset this builder.
    [[nodiscard]] virtual auto takeAnyString() -> AnyString = 0;
};

}
