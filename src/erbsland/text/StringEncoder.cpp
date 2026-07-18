// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringEncoder.hpp"

#include "impl/StringConversionTools.hpp"

namespace erbsland::text {

#define ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(TYPE)                                                                    \
    auto StringEncoderTraits<TYPE>::encode(                                                                            \
        const TYPE &source,                                                                                            \
        const StringEncoding encoding,                                                                                 \
        const StringBomMode bomMode,                                                                                   \
        const EncodingErrorMode errorMode) -> mem::ByteBlock {                                                         \
        return impl::StringConversionTools::encode(source, encoding, bomMode, errorMode);                              \
    }

ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_TRAITS

#define ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(TYPE)                                                             \
    auto StringEncoderTraits<TYPE>::encodedLength(                                                                     \
        const TYPE &source,                                                                                            \
        const StringEncoding encoding,                                                                                 \
        const StringBomMode bomMode,                                                                                   \
        const EncodingErrorMode errorMode) -> unit::ByteLength {                                                       \
        return impl::StringConversionTools::encodedLength(source, encoding, bomMode, errorMode);                       \
    }

ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS

#define ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(TYPE)                                                                 \
    auto StringEncoderTraits<TYPE>::encodeTo(                                                                          \
        const TYPE &source,                                                                                            \
        mem::RingBuffer &buffer,                                                                                       \
        const StringEncoding encoding,                                                                                 \
        const StringBomMode bomMode,                                                                                   \
        const EncodingErrorMode errorMode) -> util::Result {                                                           \
        return impl::StringConversionTools::encodeTo(source, buffer, encoding, bomMode, errorMode);                    \
    }

ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS

}
