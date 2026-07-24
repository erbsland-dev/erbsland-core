// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringDataView.hpp"
#include "U32StringEncodingTools_fwd.hpp"
#include "U32StringSharedStorage.hpp"
#include "U32Writer.hpp"

#include "../U32StringEditor.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteWriter_fwd.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../mem/RingBuffer_fwd.hpp"
#include "../../../unit/ByteLength_fwd.hpp"
#include "../../../util/Result.hpp"
#include "../../EncodingMode.hpp"
#include "../../StringBomMode.hpp"
#include "../../StringEncoding.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Binary text encoding and decoding helpers for UTF-32 string types.
/// @tested{U32StringTest}
class U32StringEncodingTools final {
public:
    /// Decoded byte layout after BOM handling.
    struct DecodeLayout final {
        mem::Endianness endianness{mem::Endianness::Little};
        std::size_t start{0};
    };

public:
    explicit constexpr U32StringEncodingTools(const U32StringDataView &data) noexcept : _data{data} {}

public:
    /// Encode the visible UTF-32 data into the requested byte encoding.
    [[nodiscard]] auto encode(StringEncoding encoding, StringBomMode bomMode) const -> mem::ByteBlock;
    /// Calculate the exact byte length produced by `encode()`.
    [[nodiscard]] auto encodedLength(StringEncoding encoding, StringBomMode bomMode) const -> unit::ByteLength;
    /// Atomically encode the visible UTF-32 data directly into a ring buffer.
    [[nodiscard]] auto encodeTo(mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) const
        -> util::Result;
    /// Decode byte data into a UTF-32 string.
    [[nodiscard]] static auto decode(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U32StringEditor;

public: // helpers
    /// Encode visible UTF-32 data as UTF-8 bytes.
    [[nodiscard]] static auto encodeUtf8(std::span<const char32_t> data, StringBomMode bomMode) -> mem::ByteBlock;
    /// Encode visible UTF-32 data as UTF-16 bytes.
    [[nodiscard]] static auto encodeUtf16(
        std::span<const char32_t> data, StringEncoding encoding, StringBomMode bomMode) -> mem::ByteBlock;
    /// Encode visible UTF-32 data as UTF-32 bytes.
    [[nodiscard]] static auto encodeUtf32(
        std::span<const char32_t> data, StringEncoding encoding, StringBomMode bomMode) -> mem::ByteBlock;
    /// Resolve the byte layout after applying BOM rules.
    [[nodiscard]] static auto resolveBomLayout(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode) -> DecodeLayout;
    /// Decode characters to UTF-32 string storage using a two-pass algorithm.
    template <typename Function>
    [[nodiscard]] static auto decodeFromCharacters(Function function) -> U32StringEditor;
    /// Decode byte data as UTF-8.
    [[nodiscard]] static auto decodeUtf8(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U32StringEditor;
    /// Decode byte data as UTF-16.
    [[nodiscard]] static auto decodeUtf16(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U32StringEditor;
    /// Decode byte data as UTF-32.
    [[nodiscard]] static auto decodeUtf32(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U32StringEditor;

private:
    U32StringDataView _data;
};

template <typename Function>
auto U32StringEncodingTools::decodeFromCharacters(Function function) -> U32StringEditor {
    auto reservedSize = unit::CpLength::zero();
    function([&](const Char character) -> void { reservedSize += utf32::encodedLength(character); });
    if (reservedSize.isZero()) {
        return U32StringEditor{};
    }

    auto storage = U32StringSharedStorage::forSize(reservedSize.toSizeT());
    const auto data = storage.dataForWrite();
    if (data == nullptr || storage.dataSize() != reservedSize.toSizeT()) {
        std::terminate();
    }

    auto writer = U32Writer{std::span{data, storage.dataSize()}};
    function([&](const Char character) -> void { writer.write(character); });
    return U32StringEditor{std::move(storage)};
}

}
