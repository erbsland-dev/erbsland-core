// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDecoder.hpp"

#include "impl/StringConversionTools.hpp"

#include "../mem/ByteBlock.hpp"

namespace erbsland::text {

StringDecoder::StringDecoder(const mem::ByteBlock &data) noexcept : _data{data} {
}

StringDecoder::StringDecoder(const mem::ByteBlockView &data) noexcept : _data{data} {
}

auto StringDecoder::decode(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const -> String {
    return toU8String(encoding, bomMode, errorMode);
}

auto StringDecoder::toU8String(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const -> U8String {
    return impl::StringConversionTools::decodeU8String(_data, encoding, bomMode, errorMode);
}

auto StringDecoder::toU16String(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const -> U16String {
    return impl::StringConversionTools::decodeU16String(_data, encoding, bomMode, errorMode);
}

auto StringDecoder::toU32String(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const -> U32String {
    return impl::StringConversionTools::decodeU32String(_data, encoding, bomMode, errorMode);
}

}
