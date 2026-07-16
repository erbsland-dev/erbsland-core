// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"
#include "U16StringEncodingTools_fwd.hpp"
#include "U16StringSharedStorage.hpp"
#include "U16Writer.hpp"

#include "../U16String.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockView_fwd.hpp"
#include "../../../mem/ByteWriter_fwd.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../EncodingErrorMode.hpp"
#include "../../StringBomMode.hpp"
#include "../../StringEncoding.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Binary text encoding and decoding helpers for UTF-16 string types.
/// @tested{U16StringTest}
class U16StringEncodingTools final {
public:
    /// Decoded byte layout after BOM handling.
    struct DecodeLayout final {
        mem::Endianness endianness{mem::Endianness::Little};
        std::size_t start{0};
    };

public:
    explicit constexpr U16StringEncodingTools(const U16StringDataView &data) noexcept : _data{data} {}

public:
    /// Encode the visible UTF-16 data into the requested byte encoding.
    [[nodiscard]] auto encode(
        StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode = EncodingErrorMode::Replace) const
        -> mem::ByteBlock;
    /// Decode byte data into a UTF-16 string.
    [[nodiscard]] static auto decode(
        const mem::ByteBlockView &data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> U16String;

public: // helpers
    /// Get the default byte order for the string encoding.
    [[nodiscard]] static auto defaultEndianness(StringEncoding encoding) noexcept -> mem::Endianness;
    /// Test if the given BOM mode should write a BOM for this encoding.
    [[nodiscard]] static auto shouldWriteBom(StringEncoding encoding, StringBomMode bomMode) noexcept -> bool;
    /// Encode visible UTF-16 data as UTF-8 bytes.
    [[nodiscard]] static auto encodeUtf8(
        std::span<const char16_t> data, StringBomMode bomMode, EncodingErrorMode errorMode) -> mem::ByteBlock;
    /// Encode visible UTF-16 data as UTF-16 bytes.
    [[nodiscard]] static auto encodeUtf16(
        std::span<const char16_t> data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    /// Encode visible UTF-16 data as UTF-32 bytes.
    [[nodiscard]] static auto encodeUtf32(
        std::span<const char16_t> data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    /// Resolve the byte layout after applying BOM rules.
    [[nodiscard]] static auto resolveBomLayout(
        const mem::ByteBlockView &data, StringEncoding encoding, StringBomMode bomMode) -> DecodeLayout;
    /// Decode characters to UTF-16 string storage using a two-pass algorithm.
    template <typename Function>
    [[nodiscard]] static auto decodeFromCharacters(Function function) -> U16String;
    /// Decode byte data as UTF-8.
    [[nodiscard]] static auto decodeUtf8(
        const mem::ByteBlockView &data, DecodeLayout layout, EncodingErrorMode errorMode) -> U16String;
    /// Decode byte data as UTF-16.
    [[nodiscard]] static auto decodeUtf16(
        const mem::ByteBlockView &data, DecodeLayout layout, EncodingErrorMode errorMode) -> U16String;
    /// Decode byte data as UTF-32.
    [[nodiscard]] static auto decodeUtf32(
        const mem::ByteBlockView &data, DecodeLayout layout, EncodingErrorMode errorMode) -> U16String;

private:
    U16StringDataView _data;
};

template <typename Function>
auto U16StringEncodingTools::decodeFromCharacters(Function function) -> U16String {
    auto reservedSize = unit::U16DataLength::zero();
    function([&](const Char character) -> void { reservedSize += utf16::encodedLength(character); });
    if (reservedSize.isZero()) {
        return U16String{};
    }

    auto storage = U16StringSharedStorage::forSize(reservedSize.toSizeT());
    const auto data = storage.dataForWrite();
    if (data == nullptr || storage.dataSize() != reservedSize.toSizeT()) {
        std::terminate();
    }

    auto writer = U16Writer{std::span{data, storage.dataSize()}};
    function([&](const Char character) -> void { writer.write(character); });
    return U16String{std::move(storage)};
}

}
