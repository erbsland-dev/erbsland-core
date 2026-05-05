// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingErrorMode.hpp"
#include "String.hpp"
#include "StringConverter_fwd.hpp"
#include "StringView.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringLiteral.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringLiteral.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringLiteral.hpp"
#include "u8/U8StringView.hpp"

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace erbsland::text {

/// Convert between Erbsland and standard string types.
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
    [[nodiscard]] auto toString(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> String {
        return toU8String(errorMode);
    }
    /// Convert to the default UTF-8 string view type.
    /// The returned view owns shared string storage if the source cannot be viewed directly.
    [[nodiscard]] auto toStringView(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> StringView {
        return StringConverterTraits<Source>::toStringView(_source, errorMode);
    }
    /// Convert to a UTF-8 string.
    [[nodiscard]] auto toU8String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U8String {
        return StringConverterTraits<Source>::toU8String(_source, errorMode);
    }
    /// Convert to a UTF-16 string.
    [[nodiscard]] auto toU16String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U16String {
        return StringConverterTraits<Source>::toU16String(_source, errorMode);
    }
    /// Convert to a UTF-32 string.
    [[nodiscard]] auto toU32String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U32String {
        return StringConverterTraits<Source>::toU32String(_source, errorMode);
    }
    /// Convert to a standard UTF-8 byte string.
    [[nodiscard]] auto toStdString(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> std::string {
        return StringConverterTraits<Source>::toStdString(_source, errorMode);
    }
    /// Convert to a standard UTF-8 string.
    [[nodiscard]] auto toStdU8String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> std::u8string {
        return StringConverterTraits<Source>::toStdU8String(_source, errorMode);
    }
    /// Convert to a standard UTF-16 string.
    [[nodiscard]] auto toStdU16String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const
        -> std::u16string {
        return StringConverterTraits<Source>::toStdU16String(_source, errorMode);
    }
    /// Convert to a standard UTF-32 string.
    [[nodiscard]] auto toStdU32String(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const
        -> std::u32string {
        return StringConverterTraits<Source>::toStdU32String(_source, errorMode);
    }
    /// Convert to a standard wide string.
    [[nodiscard]] auto toStdWString(EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> std::wstring {
        return StringConverterTraits<Source>::toStdWString(_source, errorMode);
    }

private:
    Source _source;
};

template <typename T>
StringConverter(T &&) -> StringConverter<std::decay_t<T>>;

#define ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(TYPE)                                                                 \
    template <>                                                                                                        \
    struct StringConverterTraits<TYPE> final {                                                                         \
        [[nodiscard]] static auto toStringView(const TYPE &source, EncodingErrorMode errorMode) -> StringView;         \
        [[nodiscard]] static auto toU8String(const TYPE &source, EncodingErrorMode errorMode) -> U8String;             \
        [[nodiscard]] static auto toU16String(const TYPE &source, EncodingErrorMode errorMode) -> U16String;           \
        [[nodiscard]] static auto toU32String(const TYPE &source, EncodingErrorMode errorMode) -> U32String;           \
        [[nodiscard]] static auto toStdString(const TYPE &source, EncodingErrorMode errorMode) -> std::string;         \
        [[nodiscard]] static auto toStdU8String(const TYPE &source, EncodingErrorMode errorMode) -> std::u8string;     \
        [[nodiscard]] static auto toStdU16String(const TYPE &source, EncodingErrorMode errorMode) -> std::u16string;   \
        [[nodiscard]] static auto toStdU32String(const TYPE &source, EncodingErrorMode errorMode) -> std::u32string;   \
        [[nodiscard]] static auto toStdWString(const TYPE &source, EncodingErrorMode errorMode) -> std::wstring;       \
    }

ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringView);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringLiteral<char>);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U8StringLiteral<char8_t>);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16StringView);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U16StringLiteral);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U32String);
ERBSLAND_DECLARE_STRING_CONVERTER_TRAITS(U32StringView);
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
