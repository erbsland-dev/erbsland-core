// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringView_fwd.hpp"
#include "ByteFormat.hpp"
#include "Char.hpp"
#include "FloatFormat.hpp"
#include "IntegerFormat.hpp"
#include "String_fwd.hpp"
#include "StringBuilder_fwd.hpp"
#include "StringKind.hpp"

#include "impl/FloatTraits.hpp"
#include "impl/StringBuilderBase.hpp"
#include "u16/U16String_fwd.hpp"
#include "u16/U16StringLiteral_fwd.hpp"
#include "u16/U16StringView_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringLiteral_fwd.hpp"
#include "u32/U32StringView_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringView_fwd.hpp"

#include "../math/IntegerTraits.hpp"
#include "../mem/ByteBlockView_fwd.hpp"
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
/// @seedoc{/reference/text/string_builder}
/// @tested{StringBuilderTest}
class StringBuilder final {
public:
    /// Create an empty UTF-8 string builder.
    StringBuilder();
    /// Create an empty string builder for the requested string kind.
    explicit StringBuilder(StringKind kind);

    ~StringBuilder() = default;
    StringBuilder(const StringBuilder &) = default;
    StringBuilder(StringBuilder &&) = default;
    auto operator=(const StringBuilder &) -> StringBuilder & = default;
    auto operator=(StringBuilder &&) -> StringBuilder & = default;

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
    auto append(Char character) -> StringBuilder &;
    /// Append one Unicode code point.
    auto append(char32_t codePoint) -> StringBuilder &;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpLength count) -> StringBuilder &;
    /// Append a UTF-8 string view.
    auto append(const U8StringView &text) -> StringBuilder &;
    /// Append a UTF-8 string view multiple times.
    auto append(const U8StringView &text, unit::ElementCount count) -> StringBuilder &;
    /// Append a UTF-16 string view.
    auto append(const U16StringView &text) -> StringBuilder &;
    /// Append a UTF-16 string view multiple times.
    auto append(const U16StringView &text, unit::ElementCount count) -> StringBuilder &;
    /// Append a UTF-32 string view.
    auto append(const U32StringView &text) -> StringBuilder &;
    /// Append a UTF-32 string view multiple times.
    auto append(const U32StringView &text, unit::ElementCount count) -> StringBuilder &;
    /// Append a UTF-8 `char` string literal.
    auto append(const U8StringLiteral<char> &text) -> StringBuilder &;
    /// Append a UTF-8 `char8_t` string literal.
    auto append(const U8StringLiteral<char8_t> &text) -> StringBuilder &;
    /// Append a UTF-16 string literal.
    auto append(const U16StringLiteral &text) -> StringBuilder &;
    /// Append a UTF-32 string literal.
    auto append(const U32StringLiteral &text) -> StringBuilder &;
    /// Append an "any" string view.
    auto appendAny(const AnyStringView &text) -> StringBuilder &;
    /// Append an "any" string.
    auto appendAny(const AnyString &text) -> StringBuilder &;
    /// Append an integer using the given format.
    template <math::AnyIntegerType T>
    auto appendInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat()) -> StringBuilder &;
    /// Append a floating point number using the given format.
    template <impl::AnyFloatType T>
    auto appendFloat(T value, FloatFormat format = FloatFormat::defaultFormat()) -> StringBuilder &;
    /// Append a byte block as hexadecimal text using the given format.
    auto appendByteBlock(const mem::ByteBlockView &bytes, const ByteFormat &format = ByteFormat::defaultFormat())
        -> StringBuilder &;

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
    /// Convert the builder content to one of the supported editable string types.
    template <typename T>
    [[nodiscard]] auto to() const -> T = delete;
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

public: // factory methods
    /// Create an empty UTF-8 string builder.
    [[nodiscard]] static auto u8() -> StringBuilder;
    /// Create an empty UTF-8 string builder with the given initial byte capacity.
    [[nodiscard]] static auto u8(unit::ByteLength capacity) -> StringBuilder;
    /// Create an empty UTF-16 string builder.
    [[nodiscard]] static auto u16() -> StringBuilder;
    /// Create an empty UTF-16 string builder with the given initial UTF-16 code-unit capacity.
    [[nodiscard]] static auto u16(unit::U16DataLength capacity) -> StringBuilder;
    /// Create an empty UTF-32 string builder.
    [[nodiscard]] static auto u32() -> StringBuilder;
    /// Create an empty UTF-32 string builder with the given initial code-point capacity.
    [[nodiscard]] static auto u32(unit::CpLength capacity) -> StringBuilder;
    /// Create an empty string builder with the given decoded code-point capacity.
    [[nodiscard]] static auto withCapacity(StringKind kind, unit::CpLength capacity) -> StringBuilder;
    /// Create a UTF-8 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U8StringView &initial, unit::ByteLength additionalCapacity = {})
        -> StringBuilder;
    /// Create a UTF-16 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U16StringView &initial, unit::U16DataLength additionalCapacity = {})
        -> StringBuilder;
    /// Create a UTF-32 builder initialized with text and additional native capacity.
    [[nodiscard]] static auto basedOn(const U32StringView &initial, unit::CpLength additionalCapacity = {})
        -> StringBuilder;

private:
    using BuilderPtr = mem::SharedDataPointer<impl::StringBuilderBase>;

private:
    /// Create a builder from a preconfigured backend.
    explicit StringBuilder(BuilderPtr builder);

private:
    BuilderPtr _builder; ///< The shared builder backend.
};

template <>
/// Convert this builder to a UTF-8 string.
[[nodiscard]] auto StringBuilder::to<U8String>() const -> U8String;
template <>
/// Convert this builder to a UTF-16 string.
[[nodiscard]] auto StringBuilder::to<U16String>() const -> U16String;
template <>
/// Convert this builder to a UTF-32 string.
[[nodiscard]] auto StringBuilder::to<U32String>() const -> U32String;

}

#include "StringBuilder_integer.tpp"
