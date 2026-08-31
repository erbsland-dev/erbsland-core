// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingMode.hpp"
#include "String.hpp"
#include "StringConverter_fwd.hpp"
#include "StringEditor.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u16/U16StringLiteral.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u32/U32StringLiteral.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"
#include "u8/U8StringLiteral.hpp"

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace erbsland::text {

/// Convert between Erbsland Core and standard string types.
/// @tested{StringConverterTest}
template <typename T>
class StringConverter final {
public:
    /// The source string type.
    using Source = T;

public:
    /// Create a converter from a source string value.
    explicit StringConverter(Source source) : _source{std::move(source)} {}

public:
    /// Convert to the default UTF-8 string type.
    [[nodiscard]] auto toString(EncodingMode mode = EncodingMode::Tolerant) const -> String {
        return StringConverterTraits<Source>::toU8String(_source, mode);
    }
    /// Convert to a UTF-8 string.
    [[nodiscard]] auto toU8String(EncodingMode mode = EncodingMode::Tolerant) const -> U8String {
        return StringConverterTraits<Source>::toU8String(_source, mode);
    }
    /// Convert to a UTF-16 string.
    [[nodiscard]] auto toU16String(EncodingMode mode = EncodingMode::Tolerant) const -> U16String {
        return StringConverterTraits<Source>::toU16String(_source, mode);
    }
    /// Convert to a UTF-32 string.
    [[nodiscard]] auto toU32String(EncodingMode mode = EncodingMode::Tolerant) const -> U32String {
        return StringConverterTraits<Source>::toU32String(_source, mode);
    }
    /// Convert to a standard UTF-8 byte string.
    [[nodiscard]] auto toStdString(EncodingMode mode = EncodingMode::Tolerant) const -> std::string {
        return StringConverterTraits<Source>::toStdString(_source, mode);
    }
    /// Convert to a standard UTF-8 string.
    [[nodiscard]] auto toStdU8String(EncodingMode mode = EncodingMode::Tolerant) const -> std::u8string {
        return StringConverterTraits<Source>::toStdU8String(_source, mode);
    }
    /// Convert to a standard UTF-16 string.
    [[nodiscard]] auto toStdU16String(EncodingMode mode = EncodingMode::Tolerant) const -> std::u16string {
        return StringConverterTraits<Source>::toStdU16String(_source, mode);
    }
    /// Convert to a standard UTF-32 string.
    [[nodiscard]] auto toStdU32String(EncodingMode mode = EncodingMode::Tolerant) const -> std::u32string {
        return StringConverterTraits<Source>::toStdU32String(_source, mode);
    }
    /// Convert to a standard wide string.
    [[nodiscard]] auto toStdWString(EncodingMode mode = EncodingMode::Tolerant) const -> std::wstring {
        return StringConverterTraits<Source>::toStdWString(_source, mode);
    }

private:
    Source _source;
};

template <typename T>
StringConverter(T &&) -> StringConverter<std::decay_t<T>>;

#define ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(TYPE)                                                                 \
    template <>                                                                                                        \
    struct StringConverterTraits<TYPE> final {                                                                         \
        [[nodiscard]] static auto toU8String(const TYPE &source, EncodingMode mode) -> U8String;                       \
        [[nodiscard]] static auto toU16String(const TYPE &source, EncodingMode mode) -> U16String;                     \
        [[nodiscard]] static auto toU32String(const TYPE &source, EncodingMode mode) -> U32String;                     \
        [[nodiscard]] static auto toStdString(const TYPE &source, EncodingMode mode) -> std::string;                   \
        [[nodiscard]] static auto toStdU8String(const TYPE &source, EncodingMode mode) -> std::u8string;               \
        [[nodiscard]] static auto toStdU16String(const TYPE &source, EncodingMode mode) -> std::u16string;             \
        [[nodiscard]] static auto toStdU32String(const TYPE &source, EncodingMode mode) -> std::u32string;             \
        [[nodiscard]] static auto toStdWString(const TYPE &source, EncodingMode mode) -> std::wstring;                 \
    }

ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringEditor);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringLiteral<char>);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringLiteral<char8_t>);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16StringEditor);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16StringLiteral);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U32StringEditor);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U32String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U32StringLiteral);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::string);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::string_view);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u8string);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u8string_view);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u16string);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u16string_view);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u32string);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::u32string_view);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::wstring);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(std::wstring_view);

#undef ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS

}
