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

ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8StringView);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8StringCharView);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16StringView);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16StringCharView);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32StringView);

#undef ERBSLAND_DEFINE_STRING_ENCODER_TRAITS

}
