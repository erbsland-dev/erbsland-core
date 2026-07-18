// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringBuilder_fwd.hpp"
#include "AnyStringEditor_fwd.hpp"
#include "ByteFormat.hpp"
#include "Char.hpp"
#include "FloatFormat.hpp"
#include "IntegerFormat.hpp"
#include "StringEditor_fwd.hpp"
#include "StringKind.hpp"

#include "impl/AnyStringBuilderBase.hpp"
#include "impl/FloatTraits.hpp"
#include "u16/U16String_fwd.hpp"
#include "u16/U16StringEditor_fwd.hpp"
#include "u16/U16StringLiteral_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringEditor_fwd.hpp"
#include "u32/U32StringLiteral_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringEditor_fwd.hpp"

#include "../math/IntegerTraits.hpp"
#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/SharedDataPointer.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/U16DataLength.hpp"

#include <cstddef>

namespace erbsland::text {

/// A decoded-character builder for all string encodings.
///
/// The builder creates UTF-8, UTF-16, or UTF-32 strings through one API.
/// Copies keep an independent builder state.
/// @seedoc{/reference/text/any_string_builder}
/// @tested{AnyStringBuilderTest}
class AnyStringBuilder final {
public:
    /// Create an empty UTF-8 string builder.
    AnyStringBuilder();
    /// Create an empty string builder for the requested string kind.
    explicit AnyStringBuilder(StringKind kind);

    ~AnyStringBuilder() = default;
    AnyStringBuilder(const AnyStringBuilder &) = default;
    AnyStringBuilder(AnyStringBuilder &&) = default;
    auto operator=(const AnyStringBuilder &) -> AnyStringBuilder & = default;
    auto operator=(AnyStringBuilder &&) -> AnyStringBuilder & = default;

public: // accessors
    /// Get the target string kind.
    [[nodiscard]] auto kind() const noexcept -> StringKind;
    /// Get the current decoded code-point length.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;

public: // tests
    /// Test if this builder is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;

public: // modifiers
    /// Clear all built text while keeping the target kind.
    void clear() noexcept;
    /// Append one Unicode code point.
    auto append(Char character) -> AnyStringBuilder &;
    /// Append one Unicode code point.
    auto append(char32_t codePoint) -> AnyStringBuilder &;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpLength count) -> AnyStringBuilder &;
    /// Append a UTF-8 read-only string.
    auto append(const U8String &text) -> AnyStringBuilder &;
    /// Append a UTF-8 read-only string multiple times.
    auto append(const U8String &text, unit::ElementCount count) -> AnyStringBuilder &;
    /// Append a UTF-16 read-only string.
    auto append(const U16String &text) -> AnyStringBuilder &;
    /// Append a UTF-16 read-only string multiple times.
    auto append(const U16String &text, unit::ElementCount count) -> AnyStringBuilder &;
    /// Append a UTF-32 read-only string.
    auto append(const U32String &text) -> AnyStringBuilder &;
    /// Append a UTF-32 read-only string multiple times.
    auto append(const U32String &text, unit::ElementCount count) -> AnyStringBuilder &;
    /// Append a UTF-8 `char` string literal.
    auto append(const U8StringLiteral<char> &text) -> AnyStringBuilder &;
    /// Append a UTF-8 `char8_t` string literal.
    auto append(const U8StringLiteral<char8_t> &text) -> AnyStringBuilder &;
    /// Append a UTF-16 string literal.
    auto append(const U16StringLiteral &text) -> AnyStringBuilder &;
    /// Append a UTF-32 string literal.
    auto append(const U32StringLiteral &text) -> AnyStringBuilder &;
    /// Append an "any" read-only string.
    auto appendAny(const AnyString &text) -> AnyStringBuilder &;
    /// Append an "any" string.
    auto appendAny(const AnyStringEditor &text) -> AnyStringBuilder &;
    /// Append an integer using the given format.
    template <math::AnyIntegerType T>
    auto appendInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat()) -> AnyStringBuilder &;
    /// Append a floating point number using the given format.
    template <impl::AnyFloatType T>
    auto appendFloat(T value, FloatFormat format = FloatFormat::defaultFormat()) -> AnyStringBuilder &;
    /// Append a byte block as hexadecimal text using the given format.
    auto appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format = ByteFormat::defaultFormat())
        -> AnyStringBuilder &;

public: // conversion
    /// Create a UTF-8 string copy.
    [[nodiscard]] auto toU8String() const -> U8String;
    /// @overload
    [[nodiscard]] auto toString() const -> String;
    /// Create a UTF-16 string copy.
    [[nodiscard]] auto toU16String() const -> U16String;
    /// Create a UTF-32 string copy.
    [[nodiscard]] auto toU32String() const -> U32String;
    /// Create an "any" string copy.
    [[nodiscard]] auto toAnyString() const -> AnyString;
    /// Create an editable UTF-8 string copy.
    [[nodiscard]] auto toU8StringEditor() const -> U8StringEditor;
    /// @overload
    [[nodiscard]] auto toStringEditor() const -> StringEditor;
    /// Create an editable UTF-16 string copy.
    [[nodiscard]] auto toU16StringEditor() const -> U16StringEditor;
    /// Create an editable UTF-32 string copy.
    [[nodiscard]] auto toU32StringEditor() const -> U32StringEditor;
    /// Create an editable type-erased string copy.
    [[nodiscard]] auto toAnyStringEditor() const -> AnyStringEditor;
    /// Convert the builder content to one of the supported editable string types.
    template <typename T>
    [[nodiscard]] auto toEditor() const -> T = delete;
    /// Move out a UTF-8 string and reset this builder.
    [[nodiscard]] auto takeU8String() -> U8String;
    /// @overload
    [[nodiscard]] auto takeString() -> String;
    /// Move out a UTF-16 string and reset this builder.
    [[nodiscard]] auto takeU16String() -> U16String;
    /// Move out a UTF-32 string and reset this builder.
    [[nodiscard]] auto takeU32String() -> U32String;
    /// Move out "any" string and reset this builder.
    [[nodiscard]] auto takeAnyString() -> AnyString;
    /// Move out an editable UTF-8 string and reset this builder.
    [[nodiscard]] auto takeU8StringEditor() -> U8StringEditor;
    /// @overload
    [[nodiscard]] auto takeStringEditor() -> StringEditor;
    /// Move out an editable UTF-16 string and reset this builder.
    [[nodiscard]] auto takeU16StringEditor() -> U16StringEditor;
    /// Move out an editable UTF-32 string and reset this builder.
    [[nodiscard]] auto takeU32StringEditor() -> U32StringEditor;
    /// Move out an editable type-erased string and reset this builder.
    [[nodiscard]] auto takeAnyStringEditor() -> AnyStringEditor;

public: // factory methods
    /// Create an empty UTF-8 string builder.
    [[nodiscard]] static auto u8() -> AnyStringBuilder;
    /// Create an empty UTF-8 string builder with the given initial byte capacity.
    [[nodiscard]] static auto u8(unit::ByteLength capacity) -> AnyStringBuilder;
    /// Create an empty UTF-16 string builder.
    [[nodiscard]] static auto u16() -> AnyStringBuilder;
    /// Create an empty UTF-16 string builder with the given initial UTF-16 code-unit capacity.
    [[nodiscard]] static auto u16(unit::U16DataLength capacity) -> AnyStringBuilder;
    /// Create an empty UTF-32 string builder.
    [[nodiscard]] static auto u32() -> AnyStringBuilder;
    /// Create an empty UTF-32 string builder with the given initial code-point capacity.
    [[nodiscard]] static auto u32(unit::CpLength capacity) -> AnyStringBuilder;
    /// Create an empty string builder with the given decoded code-point capacity.
    [[nodiscard]] static auto withCapacity(StringKind kind, unit::CpLength capacity) -> AnyStringBuilder;
    /// Create a UTF-8 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U8String &initial, unit::ByteLength additionalCapacity = {})
        -> AnyStringBuilder;
    /// Create a UTF-16 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U16String &initial, unit::U16DataLength additionalCapacity = {})
        -> AnyStringBuilder;
    /// Create a UTF-32 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U32String &initial, unit::CpLength additionalCapacity = {})
        -> AnyStringBuilder;

private:
    using BuilderPtr = mem::SharedDataPointer<impl::AnyStringBuilderBase>;

private:
    /// Create a builder from a preconfigured backend.
    explicit AnyStringBuilder(BuilderPtr builder);

private:
    BuilderPtr _builder; ///< The shared builder backend.
};

template <>
/// Convert this builder to a UTF-8 string.
[[nodiscard]] auto AnyStringBuilder::toEditor<U8StringEditor>() const -> U8StringEditor;
template <>
/// Convert this builder to a UTF-16 string.
[[nodiscard]] auto AnyStringBuilder::toEditor<U16StringEditor>() const -> U16StringEditor;
template <>
/// Convert this builder to a UTF-32 string.
[[nodiscard]] auto AnyStringBuilder::toEditor<U32StringEditor>() const -> U32StringEditor;

}

#include "AnyStringBuilder_integer.tpp"
