// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Random.hpp"

#include "../err/OutOfRangeError.hpp"
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

using namespace text::literals;

auto Random::buildString(const unit::CpLength length, const text::CharSet &characters) -> text::String {
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
        throw err::OutOfRangeError{"Random string exceeds size bounds"_el};
    }
    const auto capacityValue = characterCount * maximumCharacterSize;
    if (text::impl::UnsafeU8StringBuffer::wouldExceedCapacity(capacityValue + 1U)) {
        throw err::OutOfRangeError{"Random string exceeds size bounds"_el};
    }
    const auto capacity = unit::ByteLength::fromSizeTOrThrow(capacityValue);
    auto buffer = text::impl::UnsafeU8StringBuffer{capacity, isSecure()};
    auto writer = text::impl::U8Writer{std::span<char>{buffer.data(), capacity.toSizeT()}};
    for (auto i = unit::CpLength{}; i < length; ++i) {
        writer.write(selectElement(choices));
    }
    return text::String{buffer.take(unit::ByteLength::fromSizeTOrThrow(writer.position()))};
}

auto Random::buildByteBlock(const unit::ByteLength length) -> mem::ByteBlock {
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
    return mem::ByteBlock{buffer.take(length)};
}

auto Random::buildByteBuffer(const unit::ByteLength length) -> mem::ByteBuffer {
    auto result = mem::ByteBuffer{};
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

auto Random::selectIndex(const unit::ItemCount count) -> unit::ItemIndex {
    if (count.isZero() || count.isInfinite()) {
        return unit::ItemIndex::noIndex();
    }
    const auto rawIndex = selectInteger<unit::ItemIndex::Value>(0U, count.toRawValue() - 1U);
    return unit::ItemIndex{rawIndex};
}

}
