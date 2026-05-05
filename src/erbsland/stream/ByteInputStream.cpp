// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteInputStream.hpp"

#include "../mem/ByteBlockView.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace erbsland::stream {

auto ByteInputStream::endianness() const noexcept -> mem::Endianness {
    return _endianness;
}

void ByteInputStream::setEndianness(const mem::Endianness endianness) noexcept {
    _endianness = endianness;
}

auto ByteInputStream::read(const unit::ByteLength maximumLength) -> mem::ByteBlock {
    if (maximumLength.isZero()) {
        return {};
    }
    auto buffer = std::vector<mem::Byte>(maximumLength.toSizeTOrThrow());
    const auto readLength = read(std::span<mem::Byte>{buffer});
    if (readLength.isZero()) {
        return {};
    }
    return mem::ByteBlock{std::span<const mem::Byte>{buffer.data(), readLength.toSizeT()}};
}

auto ByteInputStream::readExact(const unit::ByteLength length) -> std::optional<mem::ByteBlock> {
    if (length.isZero()) {
        return mem::ByteBlock{};
    }
    auto buffer = std::vector<mem::Byte>(length.toSizeTOrThrow());
    auto position = std::size_t{0};
    while (position < buffer.size()) {
        const auto readLength = read(std::span<mem::Byte>{buffer.data() + position, buffer.size() - position});
        if (readLength.isZero()) {
            return std::nullopt;
        }
        position += readLength.toSizeT();
    }
    return mem::ByteBlock{std::span<const mem::Byte>{buffer}};
}

auto ByteInputStream::readExactOrThrow(const unit::ByteLength length) -> mem::ByteBlock {
    const auto result = readExact(length);
    if (!result.has_value()) {
        throw err::StreamError{"Unexpected end of byte stream."};
    }
    return *result;
}

auto ByteInputStream::readByte() -> std::optional<mem::Byte> {
    auto byte = mem::Byte{};
    const auto readLength = read(std::span<mem::Byte>{&byte, 1U});
    if (readLength.isZero()) {
        return std::nullopt;
    }
    return byte;
}

auto ByteInputStream::readByteOrThrow() -> mem::Byte {
    const auto result = readByte();
    if (!result.has_value()) {
        throw err::StreamError{"Unexpected end of byte stream."};
    }
    return *result;
}

auto ByteInputStream::readAll() -> mem::ByteBlock {
    constexpr auto cBufferSize = std::size_t{16U * 1024U};

    auto result = mem::ByteBlock{};
    auto buffer = std::array<mem::Byte, cBufferSize>{};
    while (true) {
        const auto readLength = read(std::span<mem::Byte>{buffer});
        if (readLength.isZero()) {
            break;
        }
        const auto block = mem::ByteBlock{std::span<const mem::Byte>{buffer.data(), readLength.toSizeT()}};
        result.append(mem::ByteBlockView{block});
    }
    return result;
}

}
