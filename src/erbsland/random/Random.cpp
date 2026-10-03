// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Random.hpp"

#include "../err/OutOfRangeFailure.hpp"
#include "../math/SaturatingMath.hpp"
#include "../mem/impl/SecureErase.hpp"
#include "../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../mem/impl/UnsafeByteBufferAccess.hpp"
#include "../text/impl/UnsafeU8StringBuffer.hpp"
#include "../text/Literals.hpp"
#include "../text/String.hpp"
#include "../text/u8/impl/U8StringData.hpp"
#include "../text/u8/impl/U8Writer.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::random {

using namespace mem;
using namespace text;
using namespace unit;
using namespace text::literals;

auto Random::buildString(const CpCount length, const CharSet &characters) -> String {
    if (length.isZero() || length.isInfinite() || characters.isEmpty()) {
        return {};
    }
    const auto choices = characters.toList();
    auto maximumCharacterSize = std::size_t{0};
    for (const auto character : choices) {
        maximumCharacterSize = std::max(maximumCharacterSize, character.utf8Size().toSizeT());
    }
    const auto characterCount = length.toSizeTOrThrow();
    if (math::willMultiplyOverflow(characterCount, maximumCharacterSize)) {
        throw err::OutOfRangeFailure{"Random string exceeds size bounds"_el};
    }
    const auto capacityValue = characterCount * maximumCharacterSize;
    if (text::impl::UnsafeU8StringBuffer::wouldExceedCapacity(capacityValue + 1U)) {
        throw err::OutOfRangeFailure{"Random string exceeds size bounds"_el};
    }
    const auto capacity = ByteCount::fromSizeTOrThrow(capacityValue);
    auto buffer = text::impl::UnsafeU8StringBuffer{capacity, isSecure()};
    auto writer = text::impl::U8Writer{std::span<char>{buffer.data(), capacity.toSizeT()}};
    for (auto i = CpCount{}; i < length; ++i) {
        writer.write(selectElement(choices));
    }
    return String{buffer.take(ByteCount::fromSizeTOrThrow(writer.position()))};
}

auto Random::buildByteBlock(const ByteCount length) -> ByteBlock {
    if (length.isZero() || length.isInfinite()) {
        return {};
    }
    const auto secure = isSecure();
    auto buffer = mem::impl::UnsafeByteBlockBuffer{length, secure};
    try {
        fillBytes(std::as_writable_bytes(buffer.data()));
    } catch (...) {
        if (!secure) {
            mem::impl::secureErase(std::as_writable_bytes(buffer.data()));
        }
        throw;
    }
    return ByteBlock{buffer.take(length)};
}

auto Random::buildByteBuffer(const ByteCount length) -> ByteBuffer {
    auto result = ByteBuffer{};
    result.setSensitive(isSecure());
    if (length.isZero() || length.isInfinite()) {
        return result;
    }
    result.resize(length);
    auto access = mem::impl::UnsafeByteBufferAccess{result};
    try {
        fillBytes(std::as_writable_bytes(access.writableData()));
    } catch (...) {
        result.secureErase();
        throw;
    }
    return result;
}

auto Random::selectIndex(const ItemCount count) -> ItemIndex {
    if (count.isZero() || count.isInfinite()) {
        return ItemIndex::noIndex();
    }
    const auto rawIndex = selectInteger<ItemIndex::Value>(0U, count.toRawValue() - 1U);
    return ItemIndex{rawIndex, ItemIndex::Unchecked{}};
}

}
