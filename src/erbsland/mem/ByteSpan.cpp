// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteSpan.hpp"

namespace erbsland::mem {

static_assert(sizeof(Byte) == sizeof(std::byte));
static_assert(alignof(Byte) == alignof(std::byte));
static_assert(sizeof(Byte) == sizeof(uint8_t));
static_assert(alignof(Byte) == alignof(uint8_t));
static_assert(sizeof(Byte) == sizeof(char));
static_assert(alignof(Byte) == alignof(char));

auto toByteSpan(const std::span<std::byte> span) noexcept -> ByteSpan {
    return {reinterpret_cast<Byte *>(span.data()), span.size()};
}

auto toByteSpan(const std::span<uint8_t> span) noexcept -> ByteSpan {
    return {reinterpret_cast<Byte *>(span.data()), span.size()};
}

auto toByteSpan(const std::span<char> span) noexcept -> ByteSpan {
    return {reinterpret_cast<Byte *>(span.data()), span.size()};
}

auto toConstByteSpan(const std::span<const std::byte> span) noexcept -> ConstByteSpan {
    return {reinterpret_cast<const Byte *>(span.data()), span.size()};
}

auto toConstByteSpan(const std::span<const uint8_t> span) noexcept -> ConstByteSpan {
    return {reinterpret_cast<const Byte *>(span.data()), span.size()};
}

auto toConstByteSpan(const std::span<const char> span) noexcept -> ConstByteSpan {
    return {reinterpret_cast<const Byte *>(span.data()), span.size()};
}

}
