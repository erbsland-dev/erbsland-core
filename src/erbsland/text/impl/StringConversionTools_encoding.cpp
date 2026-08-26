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
template <typename T>
[[nodiscard]] auto StringConversionTools::isMatchingRepresentation(const StringEncoding encoding) noexcept -> bool {
    if constexpr (std::same_as<T, char>) {
        return encoding.isUtf8();
    } else if constexpr (std::same_as<T, char16_t>) {
        const auto nativeEndianness =
            std::endian::native == std::endian::little ? mem::Endianness::Little : mem::Endianness::Big;
        return encoding.isUtf16() && encoding.endianness() == nativeEndianness;
    } else if constexpr (std::same_as<T, char32_t>) {
        const auto nativeEndianness =
            std::endian::native == std::endian::little ? mem::Endianness::Little : mem::Endianness::Big;
        return encoding.isUtf32() && encoding.endianness() == nativeEndianness;
    } else {
        return false;
    }
}

template <typename T>
[[nodiscard]] auto StringConversionTools::asByteSpan(const std::span<const T> source) noexcept -> mem::ConstByteSpan {
    return {reinterpret_cast<const mem::Byte *>(source.data()), source.size_bytes()};
}

template <typename T>
[[nodiscard]] auto StringConversionTools::rawEncodedLength(
    const std::span<const T> source, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {
    auto result = encoding.bomLength(bomMode);
    result.addOrThrow(unit::ByteLength::fromSizeT(source.size_bytes()));
    return result;
}

void StringConversionTools::copyBytes(
    const mem::ByteSpan destination, std::size_t &offset, const mem::ConstByteSpan source) noexcept {
    if (!source.empty()) {
        std::memcpy(destination.data() + offset, source.data(), source.size());
        offset += source.size();
    }
}

template <typename T>
[[nodiscard]] auto StringConversionTools::encodeRaw(
    const std::span<const T> source, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    const auto length = rawEncodedLength(source, encoding, bomMode);
    auto buffer = mem::impl::UnsafeByteBlockBuffer{length};
    auto offset = std::size_t{0U};
    copyBytes(buffer.data(), offset, encoding.bomBytes(bomMode));
    copyBytes(buffer.data(), offset, asByteSpan(source));
    return buffer.take(length);
}

template <typename T>
[[nodiscard]] auto StringConversionTools::encodeRawTo(
    const std::span<const T> source,
    mem::RingBuffer &buffer,
    const StringEncoding encoding,
    const StringBomMode bomMode) -> util::Result {
    const auto length = rawEncodedLength(source, encoding, bomMode);
    if (isFailure(buffer.reserveAdditional(length))) {
        return util::Result::Failure;
    }
    auto access = mem::impl::UnsafeRingBufferAccess{buffer};
    const auto writable = access.writableSpans();
    auto spanIndex = std::size_t{0U};
    auto spanOffset = std::size_t{0U};
    auto write = [&](const mem::ConstByteSpan bytes) -> void {
        auto sourceOffset = std::size_t{0U};
        while (sourceOffset < bytes.size()) {
            if (spanOffset == writable[spanIndex].size()) {
                ++spanIndex;
                spanOffset = 0U;
            }
            const auto count = std::min(bytes.size() - sourceOffset, writable[spanIndex].size() - spanOffset);
            std::memcpy(writable[spanIndex].data() + spanOffset, bytes.data() + sourceOffset, count);
            sourceOffset += count;
            spanOffset += count;
        }
    };
    write(encoding.bomBytes(bomMode));
    write(asByteSpan(source));
    access.commitWritten(length);
    return util::Result::Success;
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodeString(
    const tDataView dataView, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    if (isMatchingRepresentation<tData>(encoding)) {
        return encodeRaw<tData>(dataView.dataSpan(), encoding, bomMode);
    }
    return tTools{dataView}.encode(encoding, bomMode);
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodedStringLength(
    const tDataView dataView, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {
    if (isMatchingRepresentation<tData>(encoding)) {
        return rawEncodedLength<tData>(dataView.dataSpan(), encoding, bomMode);
    }
    return tTools{dataView}.encodedLength(encoding, bomMode);
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodeStringTo(
    const tDataView dataView, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)
    -> util::Result {
    if (isMatchingRepresentation<tData>(encoding)) {
        return encodeRawTo<tData>(dataView.dataSpan(), buffer, encoding, bomMode);
    }
    return tTools{dataView}.encodeTo(buffer, encoding, bomMode);
}

#define ERBSLAND_DEFINE_STRING_CONVERSION(TYPE, DATA_TYPE, TOOLS)                                                      \
    auto StringConversionTools::encode(const TYPE &str, const StringEncoding encoding, const StringBomMode bomMode)    \
        -> mem::ByteBlock {                                                                                            \
        return encodeString<DATA_TYPE, TOOLS>(str.dataView(), encoding, bomMode);                                      \
    }                                                                                                                  \
    auto StringConversionTools::encodedLength(                                                                         \
        const TYPE &str, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {             \
        return encodedStringLength<DATA_TYPE, TOOLS>(str.dataView(), encoding, bomMode);                               \
    }                                                                                                                  \
    auto StringConversionTools::encodeTo(                                                                              \
        const TYPE &str, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)          \
        -> util::Result {                                                                                              \
        return encodeStringTo<DATA_TYPE, TOOLS>(str.dataView(), buffer, encoding, bomMode);                            \
    }

ERBSLAND_DEFINE_STRING_CONVERSION(U8StringEditor, char, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U8String, char, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U16StringEditor, char16_t, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U16String, char16_t, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U32StringEditor, char32_t, U32StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U32String, char32_t, U32StringEncodingTools);

#undef ERBSLAND_DEFINE_STRING_CONVERSION
}
