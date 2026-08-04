// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"
#include "U16StringEncodingTools_fwd.hpp"
#include "U16StringSharedStorage.hpp"
#include "U16Writer.hpp"

#include "../U16StringEditor.hpp"

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
    /// Create encoding tools for `data`.
    explicit constexpr U16StringEncodingTools(const U16StringDataView &data) noexcept : _data{data} {}

public:
    /// Encode the visible UTF-16 data into the requested byte encoding.
    [[nodiscard]] auto encode(StringEncoding encoding, StringBomMode bomMode) const -> mem::ByteBlock;
    /// Calculate the exact byte length produced by `encode()`.
    [[nodiscard]] auto encodedLength(StringEncoding encoding, StringBomMode bomMode) const -> unit::ByteLength;
    /// Atomically encode the visible UTF-16 data directly into a ring buffer.
    [[nodiscard]] auto encodeTo(mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) const
        -> util::Result;
    /// Decode byte data into a UTF-16 string.
    [[nodiscard]] static auto decode(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U16StringEditor;

public: // helpers
    /// Encode visible UTF-16 data as UTF-8 bytes.
    [[nodiscard]] static auto encodeUtf8(std::span<const char16_t> data, StringBomMode bomMode) -> mem::ByteBlock;
    /// Encode visible UTF-16 data as UTF-16 bytes.
    [[nodiscard]] static auto encodeUtf16(
        std::span<const char16_t> data, StringEncoding encoding, StringBomMode bomMode) -> mem::ByteBlock;
    /// Encode visible UTF-16 data as UTF-32 bytes.
    [[nodiscard]] static auto encodeUtf32(
        std::span<const char16_t> data, StringEncoding encoding, StringBomMode bomMode) -> mem::ByteBlock;
    /// Resolve the byte layout after applying BOM rules.
    [[nodiscard]] static auto resolveBomLayout(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode) -> DecodeLayout;
    /// Decode characters to UTF-16 string storage using a two-pass algorithm.
    template <typename Function>
    [[nodiscard]] static auto decodeFromCharacters(Function function) -> U16StringEditor;
    /// Decode byte data as UTF-8.
    [[nodiscard]] static auto decodeUtf8(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U16StringEditor;
    /// Decode byte data as UTF-16.
    [[nodiscard]] static auto decodeUtf16(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U16StringEditor;
    /// Decode byte data as UTF-32.
    [[nodiscard]] static auto decodeUtf32(const mem::ByteBlock &data, DecodeLayout layout, EncodingMode mode)
        -> U16StringEditor;

private:
    U16StringDataView _data;
};

/// Decode characters supplied by a callback into a UTF-16 editor.
template <typename Function>
auto U16StringEncodingTools::decodeFromCharacters(Function function) -> U16StringEditor {
    auto reservedSize = unit::U16DataLength::zero();
    function([&](const Char character) -> void { reservedSize += utf16::encodedLength(character); });
    if (reservedSize.isZero()) {
        return U16StringEditor{};
    }

    auto storage = U16StringSharedStorage::forSize(reservedSize.toSizeT());
    const auto data = storage.dataForWrite();
    if (data == nullptr || storage.dataSize() != reservedSize.toSizeT()) {
        std::terminate();
    }

    auto writer = U16Writer{std::span{data, storage.dataSize()}};
    function([&](const Char character) -> void { writer.write(character); });
    return U16StringEditor{std::move(storage)};
}

}
