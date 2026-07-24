// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char.hpp"
#include "StringBomMode.hpp"
#include "StringEncoder_fwd.hpp"
#include "StringEncoding.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/RingBuffer_fwd.hpp"
#include "../unit/ByteLength_fwd.hpp"
#include "../util/Result.hpp"

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
    /// Matching native representations are copied without validation.
    /// Actual transcoding replaces malformed source sequences.
    [[nodiscard]] auto encode(StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> mem::ByteBlock {
        return StringEncoderTraits<Source>::encode(*_source, encoding, bomMode);
    }
    /// Calculate the exact byte length produced by `encode()` without allocating the encoded byte block.
    /// Matching native representations are measured without validation.
    /// @throws err::OverflowError If the encoded length exceeds the supported finite byte length.
    [[nodiscard]] auto encodedLength(StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> unit::ByteLength {
        return StringEncoderTraits<Source>::encodedLength(*_source, encoding, bomMode);
    }
    /// Atomically encode the source directly into a ring buffer.
    /// Each call independently applies `bomMode`; stateful streams must suppress the BOM after their first write.
    /// Matching native representations are copied without validation.
    /// Actual transcoding replaces malformed source sequences.
    /// @return Success, or failure without modification if the encoded data exceeds the remaining hard capacity.
    /// @throws err::OverflowError If the encoded length exceeds the supported finite byte length.
    [[nodiscard]] auto encodeTo(
        mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> util::Result {
        return StringEncoderTraits<Source>::encodeTo(*_source, buffer, encoding, bomMode);
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
        [[nodiscard]] static auto encodedLength(const TYPE &source, StringEncoding encoding, StringBomMode bomMode)    \
            -> unit::ByteLength;                                                                                       \
        [[nodiscard]] static auto encodeTo(                                                                            \
            const TYPE &source, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)               \
            -> util::Result;                                                                                           \
    }

ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U8StringEditor);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U8String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U16StringEditor);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U16String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U32StringEditor);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(U32String);
ERBSLAND_DECLARE_STRING_ENCODER_TRAITS(Char);

#undef ERBSLAND_DECLARE_STRING_ENCODER_TRAITS

}
