// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingErrorMode.hpp"
#include "StringBomMode.hpp"
#include "StringEncoding.hpp"
#include "StringView.hpp"

#include "../mem/RingBuffer.hpp"

namespace erbsland::text {

/// A byte ring with atomic incremental Unicode encoding.
/// The default replacement mode encodes directly into ring storage without materializing the complete encoded text.
/// @tested{TextRingBufferTest}
class TextRingBuffer final : public mem::RingBuffer {
public:
    using RingBuffer::RingBuffer;

public:
    /// Calculate the byte length produced by an encoded write.
    [[nodiscard]] static auto encodedLength(
        const StringView &text,
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) -> unit::ByteLength;
    /// Atomically encode and append text.
    /// @return `false` without modification if the encoded data exceeds the remaining hard capacity.
    auto writeEncoded(
        const StringView &text,
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) -> bool;
};

}
