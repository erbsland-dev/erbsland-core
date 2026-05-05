// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEncodingTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../U8String.hpp"

#include "../../../err/ThrowHelper.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockView.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../Char.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32Writer.hpp"

#include <cstdint>
#include <span>

namespace erbsland::text::impl {

auto U8StringEncodingTools::defaultEndianness(const StringEncoding encoding) noexcept -> mem::Endianness {
    if (encoding == StringEncoding::Utf16BigEndian || encoding == StringEncoding::Utf32BigEndian) {
        return mem::Endianness::Big;
    }
    return mem::Endianness::Little;
}

auto U8StringEncodingTools::shouldWriteBom(const StringEncoding encoding, const StringBomMode bomMode) noexcept
    -> bool {
    switch (bomMode) {
    case StringBomMode::Reject:
        return false;
    case StringBomMode::Require:
        return true;
    case StringBomMode::Automatic:
        return encoding != StringEncoding::Utf8;
    }
    return false;
}

auto U8StringEncodingTools::encodeUtf8(const std::span<const char> data, const StringBomMode bomMode)
    -> mem::ByteBlock {
    auto writer = mem::ByteWriter{};
    auto u8Writer = U8Writer{writer};
    if (shouldWriteBom(StringEncoding::Utf8, bomMode)) {
        u8Writer.writeBom();
    }
    utf8::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { u8Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::encodeUtf16(
    const std::span<const char> data, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    auto writer = mem::ByteWriter{};
    writer.setEndianness(defaultEndianness(encoding));
    auto u16Writer = U16Writer{writer};
    if (shouldWriteBom(encoding, bomMode)) {
        u16Writer.writeBom();
    }
    utf8::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { u16Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::encodeUtf32(
    const std::span<const char> data, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    auto writer = mem::ByteWriter{};
    writer.setEndianness(defaultEndianness(encoding));
    auto u32Writer = U32Writer{writer};
    if (shouldWriteBom(encoding, bomMode)) {
        u32Writer.writeBom();
    }
    utf8::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { u32Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::resolveBomLayout(
    const mem::ByteBlockView &data, const StringEncoding encoding, const StringBomMode bomMode) -> DecodeLayout {
    const auto hasUtf32LeBom = utf32::hasLittleEndianBom(data);
    const auto hasUtf32BeBom = utf32::hasBigEndianBom(data);
    const auto hasUtf8Bom = utf8::hasBom(data);
    const auto hasUtf16LeBom = !hasUtf32LeBom && utf16::hasLittleEndianBom(data);
    const auto hasUtf16BeBom = utf16::hasBigEndianBom(data);
    const auto hasBom = hasUtf32LeBom || hasUtf32BeBom || hasUtf8Bom || hasUtf16LeBom || hasUtf16BeBom;
    if (!hasBom) {
        if (bomMode == StringBomMode::Require) {
            err::throwEncodingError("Missing byte order mark");
        }
        return DecodeLayout{.endianness = defaultEndianness(encoding), .start = 0U};
    }
    if (bomMode == StringBomMode::Reject) {
        err::throwEncodingError("Unexpected byte order mark");
    }

    if (hasUtf8Bom) {
        if (encoding != StringEncoding::Utf8) {
            err::throwEncodingError("Unexpected UTF-8 byte order mark");
        }
        return DecodeLayout{.endianness = mem::Endianness::Little, .start = utf8::bomLength()};
    }
    if (hasUtf16LeBom) {
        if (!utf16::isEncoding(encoding) || encoding == StringEncoding::Utf16BigEndian) {
            err::throwEncodingError("Unexpected UTF-16 little endian byte order mark");
        }
        return DecodeLayout{.endianness = mem::Endianness::Little, .start = utf16::bomLength()};
    }
    if (hasUtf16BeBom) {
        if (!utf16::isEncoding(encoding) || encoding == StringEncoding::Utf16LittleEndian) {
            err::throwEncodingError("Unexpected UTF-16 big endian byte order mark");
        }
        return DecodeLayout{.endianness = mem::Endianness::Big, .start = utf16::bomLength()};
    }
    if (hasUtf32LeBom) {
        if (!utf32::isEncoding(encoding) || encoding == StringEncoding::Utf32BigEndian) {
            err::throwEncodingError("Unexpected UTF-32 little endian byte order mark");
        }
        return DecodeLayout{.endianness = mem::Endianness::Little, .start = utf32::bomLength()};
    }
    if (hasUtf32BeBom) {
        if (!utf32::isEncoding(encoding) || encoding == StringEncoding::Utf32LittleEndian) {
            err::throwEncodingError("Unexpected UTF-32 big endian byte order mark");
        }
        return DecodeLayout{.endianness = mem::Endianness::Big, .start = utf32::bomLength()};
    }
    return DecodeLayout{.endianness = defaultEndianness(encoding), .start = 0U};
}

auto U8StringEncodingTools::decodeUtf8(
    const mem::ByteBlockView &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U8String {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U8StringEncodingTools::decodeUtf16(
    const mem::ByteBlockView &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U8String {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U8StringEncodingTools::decodeUtf32(
    const mem::ByteBlockView &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U8String {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = mem::ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U8StringEncodingTools::encode(const StringEncoding encoding, const StringBomMode bomMode) const -> mem::ByteBlock {
    switch (encoding) {
    case StringEncoding::Utf8:
        return encodeUtf8(_data.dataSpan(), bomMode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return encodeUtf16(_data.dataSpan(), encoding, bomMode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return encodeUtf32(_data.dataSpan(), encoding, bomMode);
    }
    return {};
}

auto U8StringEncodingTools::decode(
    const mem::ByteBlockView &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U8String {
    const auto layout = resolveBomLayout(data, encoding, bomMode);
    switch (encoding) {
    case StringEncoding::Utf8:
        return decodeUtf8(data, layout, errorMode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return decodeUtf16(data, layout, errorMode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return decodeUtf32(data, layout, errorMode);
    }
    return {};
}

}
