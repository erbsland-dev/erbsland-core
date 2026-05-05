// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringBomMode.hpp"
#include "StringEncoder_fwd.hpp"
#include "StringEncoding.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringCharView.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringCharView.hpp"
#include "u8/U8StringView.hpp"

#include "../mem/ByteBlock_fwd.hpp"

#include <type_traits>

namespace erbsland::text {

/// Encode Erbsland strings as binary text data.
/// @tested{StringEncoderTest}
template <typename T>
class StringEncoder final {
public:
    /// The source string type.
    using Source = T;

public:
    /// Create an encoder for the given source string.
    explicit StringEncoder(const Source &source) noexcept : _source{&source} {}

public:
    /// Encode the source string into the requested byte encoding.
    [[nodiscard]] auto encode(StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> mem::ByteBlock {
        return StringEncoderTraits<Source>::encode(*_source, encoding, bomMode);
    }

private:
    const Source *_source;
};

template <typename T>
StringEncoder(const T &) -> StringEncoder<std::remove_cvref_t<T>>;

#define ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(TYPE)                                                                   \
    template <>                                                                                                        \
    struct StringEncoderTraits<TYPE> final {                                                                           \
        [[nodiscard]] static auto encode(const TYPE &source, StringEncoding encoding, StringBomMode bomMode)           \
            -> mem::ByteBlock;                                                                                         \
    }

ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U8String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U8StringView);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U8StringCharView);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U16String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U16StringView);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U16StringCharView);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U32String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U32StringView);

#undef ERBSLAND_DECLARE_STRING_ENCODER_TRAITS

}
