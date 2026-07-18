// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodingErrorMode.hpp"
#include "String.hpp"
#include "StringBomMode.hpp"
#include "StringDecoder_fwd.hpp"
#include "StringEncoding.hpp"

#include "u16/U16String.hpp"
#include "u32/U32String.hpp"
#include "u8/U8String.hpp"

#include "../mem/ByteBlock.hpp"
#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/ByteBlockEditor_fwd.hpp"

namespace erbsland::text {

/// Decode binary text data into Erbsland strings.
/// @tested{StringDecoderTest}
class StringDecoder final {
public:
    /// Create a decoder sharing data from a byte block editor.
    StringDecoder(const mem::ByteBlockEditor &data) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a decoder sharing data from a read-only byte block.
    StringDecoder(const mem::ByteBlock &data) noexcept; // NOLINT(*-explicit-constructor)

public:
    /// Decode to the default UTF-8 string type.
    [[nodiscard]] auto decode(
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> String;
    /// Decode to a UTF-8 string.
    [[nodiscard]] auto toU8String(
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U8String;
    /// Decode to a UTF-16 string.
    [[nodiscard]] auto toU16String(
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U16String;
    /// Decode to a UTF-32 string.
    [[nodiscard]] auto toU32String(
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace) const -> U32String;

private:
    mem::ByteBlock _data;
};

}
