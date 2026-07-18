// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Random.hpp"

#include "../mem/ByteWriter.hpp"
#include "../text/String.hpp"

#include <algorithm>
#include <array>

namespace erbsland::random {

auto Random::buildString(const unit::CpLength length, const text::CharSet &characters) -> text::String {
    if (length.isZero() || length.isInfinite() || characters.isEmpty()) {
        return {};
    }
    const auto choices = characters.toList();
    auto result = text::StringEditor{};
    for (auto i = unit::CpLength{}; i < length; ++i) {
        result.append(selectElement(choices));
    }
    return result;
}

auto Random::buildByteBlock(const unit::ByteLength length) -> mem::ByteBlock {
    if (length.isZero() || length.isInfinite()) {
        return {};
    }
    auto writer = mem::ByteWriter{};
    writer.reserve(length);
    auto remaining = length.toSizeTOrThrow();
    auto buffer = std::array<std::byte, 256>{};
    while (remaining > 0U) {
        const auto chunkSize = std::min(remaining, buffer.size());
        auto chunk = std::span<std::byte>{buffer}.first(chunkSize);
        fillBytes(chunk);
        for (const auto byte : chunk) {
            writer.writeByte(mem::Byte{static_cast<uint8_t>(byte)});
        }
        remaining -= chunkSize;
    }
    return writer.toByteBlock();
}

auto Random::selectIndex(const unit::ElementCount count) -> unit::ElementIndex {
    if (count.isZero() || count.isInfinite()) {
        return unit::ElementIndex::noIndex();
    }
    const auto rawIndex = selectInteger<unit::ElementIndex::Value>(0U, count.toRawValue() - 1U);
    return unit::ElementIndex{rawIndex};
}

}
