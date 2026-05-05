// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringConverter.hpp"

#include "impl/StringConversionTools.hpp"

namespace erbsland::text {

#define ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(TYPE)                                                                \
    auto StringConverterTraits<TYPE>::toStringView(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> StringView {                                                                                                \
        return U8StringView{toU8String(source, errorMode)};                                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return impl::StringConversionTools::toU8String(source, errorMode);                                             \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return impl::StringConversionTools::toU16String(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return impl::StringConversionTools::toU32String(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return impl::StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);    \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return impl::StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);  \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return impl::StringConversionTools::toStdU16String(                                                            \
            toU16String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return impl::StringConversionTools::toStdU32String(                                                            \
            toU32String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return impl::StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);   \
    }

#define ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS(TYPE)                                                                 \
    auto StringConverterTraits<TYPE>::toStringView(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> StringView {                                                                                                \
        static_cast<void>(errorMode);                                                                                  \
        return U8StringView{source};                                                                                   \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return impl::StringConversionTools::toU8String(source, errorMode);                                             \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return impl::StringConversionTools::toU16String(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return impl::StringConversionTools::toU32String(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return impl::StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);    \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return impl::StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);  \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return impl::StringConversionTools::toStdU16String(                                                            \
            toU16String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return impl::StringConversionTools::toStdU32String(                                                            \
            toU32String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return impl::StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);   \
    }

#define ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(TYPE, VIEW_TYPE, VIEW_EXPR)                                               \
    auto StringConverterTraits<TYPE>::toStringView(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> StringView {                                                                                                \
        return U8StringView{toU8String(source, errorMode)};                                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return impl::StringConversionTools::toU8String(VIEW_TYPE{VIEW_EXPR}, errorMode);                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return impl::StringConversionTools::toU16String(VIEW_TYPE{VIEW_EXPR}, errorMode);                              \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return impl::StringConversionTools::toU32String(VIEW_TYPE{VIEW_EXPR}, errorMode);                              \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return impl::StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);    \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return impl::StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);  \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return impl::StringConversionTools::toStdU16String(                                                            \
            toU16String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return impl::StringConversionTools::toStdU32String(                                                            \
            toU32String(source, errorMode), EncodingErrorMode::Replace);                                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return impl::StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);   \
    }

ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS(U8String);
ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS(U8StringView);
ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS(U8StringLiteral<char>);
ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS(U8StringLiteral<char8_t>);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16String);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16StringView);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16StringLiteral);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U32String);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U32StringView);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U32StringLiteral);

ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::string, std::string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::string_view, std::string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u8string, std::u8string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u8string_view, std::u8string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u16string, std::u16string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u16string_view, std::u16string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u32string, std::u32string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::u32string_view, std::u32string_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::wstring, std::wstring_view, source);
ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(std::wstring_view, std::wstring_view, source);

#undef ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS
#undef ERBSLAND_DEFINE_U8_VIEW_CONVERTER_TRAITS
#undef ERBSLAND_DEFINE_STD_CONVERTER_TRAITS

}
