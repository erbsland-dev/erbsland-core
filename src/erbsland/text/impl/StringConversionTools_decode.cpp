// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringConversionTools.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u16/impl/U16StringEncodingTools.hpp"
#include "../u16/impl/U16StringReadTools.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/impl/U32StringEncodingTools.hpp"
#include "../u32/impl/U32StringReadTools.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/impl/U8StringEncodingTools.hpp"
#include "../u8/impl/U8StringReadTools.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../mem/impl/UnsafeRingBufferAccess.hpp"
#include "../../mem/RingBuffer.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <span>

namespace erbsland::text::impl {
auto StringConversionTools::decodeU8String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U8StringEditor {
    return U8StringEncodingTools::decode(data, encoding, bomMode, mode);
}

auto StringConversionTools::decodeU16String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U16StringEditor {
    return U16StringEncodingTools::decode(data, encoding, bomMode, mode);
}

auto StringConversionTools::decodeU32String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U32StringEditor {
    return U32StringEncodingTools::decode(data, encoding, bomMode, mode);
}

void StringConversionTools::validateEncodedData(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode) {
    U8StringEncodingTools::validate(data, encoding, bomMode);
}

}
