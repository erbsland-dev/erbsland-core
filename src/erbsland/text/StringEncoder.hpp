// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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
template <impl::AnyStringOrStringEditorType T>
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
        -> mem::ByteBlock;
    /// Calculate the exact byte length produced by `encode()` without allocating the encoded byte block.
    /// Matching native representations are measured without validation.
    /// @throws err::OverflowError If the encoded length exceeds the supported finite byte length.
    [[nodiscard]] auto encodedLength(StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> unit::ByteLength;
    /// Atomically encode the source directly into a ring buffer.
    /// Each call independently applies `bomMode`; stateful streams must suppress the BOM after their first write.
    /// Matching native representations are copied without validation.
    /// Actual transcoding replaces malformed source sequences.
    /// @return Success, or failure without modification if the encoded data exceeds the remaining hard capacity.
    /// @throws err::OverflowError If the encoded length exceeds the supported finite byte length.
    [[nodiscard]] auto encodeTo(
        mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode = StringBomMode::Automatic) const
        -> util::Result;

private:
    const Source *_source;
};

/// Deduce the encoder source type.
template <impl::AnyStringOrStringEditorType T>
StringEncoder(const T &) -> StringEncoder<std::remove_cvref_t<T>>;

extern template class StringEncoder<U8StringEditor>;
extern template class StringEncoder<U8String>;
extern template class StringEncoder<U16StringEditor>;
extern template class StringEncoder<U16String>;
extern template class StringEncoder<U32StringEditor>;
extern template class StringEncoder<U32String>;

}
