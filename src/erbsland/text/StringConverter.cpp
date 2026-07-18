// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringConverter.hpp"

#include "impl/StringConversionTools.hpp"

namespace erbsland::text {

using impl::StringConversionTools;

#define ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(TYPE)                                                                \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return StringConversionTools::toU8StringEditor(source, errorMode);                                             \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return StringConversionTools::toU16StringEditor(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return StringConversionTools::toU32StringEditor(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);          \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);        \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return StringConversionTools::toStdU16String(toU16String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return StringConversionTools::toStdU32String(toU32String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);         \
    }

#define ERBSLAND_DEFINE_U8_CONVERTER_TRAITS(TYPE)                                                                      \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return StringConversionTools::toU8StringEditor(source, errorMode);                                             \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return StringConversionTools::toU16StringEditor(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return StringConversionTools::toU32StringEditor(source, errorMode);                                            \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);          \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);        \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return StringConversionTools::toStdU16String(toU16String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return StringConversionTools::toStdU32String(toU32String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);         \
    }

#define ERBSLAND_DEFINE_STD_CONVERTER_TRAITS(TYPE, VIEW_TYPE, VIEW_EXPR)                                               \
    auto StringConverterTraits<TYPE>::toU8String(const TYPE &source, const EncodingErrorMode errorMode) -> U8String {  \
        return StringConversionTools::toU8StringEditor(VIEW_TYPE{VIEW_EXPR}, errorMode);                               \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU16String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U16String {                                                                                                 \
        return StringConversionTools::toU16StringEditor(VIEW_TYPE{VIEW_EXPR}, errorMode);                              \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toU32String(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> U32String {                                                                                                 \
        return StringConversionTools::toU32StringEditor(VIEW_TYPE{VIEW_EXPR}, errorMode);                              \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdString(const TYPE &source, const EncodingErrorMode errorMode)               \
        -> std::string {                                                                                               \
        return StringConversionTools::toStdString(toU8String(source, errorMode), EncodingErrorMode::Replace);          \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU8String(const TYPE &source, const EncodingErrorMode errorMode)             \
        -> std::u8string {                                                                                             \
        return StringConversionTools::toStdU8String(toU8String(source, errorMode), EncodingErrorMode::Replace);        \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU16String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u16string {                                                                                            \
        return StringConversionTools::toStdU16String(toU16String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdU32String(const TYPE &source, const EncodingErrorMode errorMode)            \
        -> std::u32string {                                                                                            \
        return StringConversionTools::toStdU32String(toU32String(source, errorMode), EncodingErrorMode::Replace);      \
    }                                                                                                                  \
    auto StringConverterTraits<TYPE>::toStdWString(const TYPE &source, const EncodingErrorMode errorMode)              \
        -> std::wstring {                                                                                              \
        return StringConversionTools::toStdWString(toU8String(source, errorMode), EncodingErrorMode::Replace);         \
    }

ERBSLAND_DEFINE_U8_CONVERTER_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_U8_CONVERTER_TRAITS(U8String);
ERBSLAND_DEFINE_U8_CONVERTER_TRAITS(U8StringLiteral<char>);
ERBSLAND_DEFINE_U8_CONVERTER_TRAITS(U8StringLiteral<char8_t>);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16String);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U16StringLiteral);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_ERBSLAND_CONVERTER_TRAITS(U32String);
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
#undef ERBSLAND_DEFINE_U8_CONVERTER_TRAITS
#undef ERBSLAND_DEFINE_STD_CONVERTER_TRAITS

}
