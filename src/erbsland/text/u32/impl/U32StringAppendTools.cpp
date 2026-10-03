// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringAppendTools.hpp"

#include "U32Encoding.hpp"
#include "U32Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../impl/UnsafeU16StringAccess.hpp"
#include "../../impl/UnsafeU32StringAccess.hpp"
#include "../../impl/UnsafeU8StringAccess.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16StringDataView.hpp"
#include "../../u8/impl/U8Encoding.hpp"
#include "../../u8/impl/U8StringDataView.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

using namespace unit;

auto U32StringAppendTools::append(const U32StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, source.size());
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size() * sizeof(char32_t));
    _storage.resize(newSize);
    return CpCount::fromSizeT(source.size());
}

auto U32StringAppendTools::append(const U32StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendSize = U32StringSharedStorage::checkedMultiplySize(source.size(), countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size() * sizeof(char32_t));
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(CpCount::fromSizeTOrThrow(source.size()), countSize);
}

auto U32StringAppendTools::append(const U8StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendedLength.toSizeTOrThrow());
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf8::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const U8StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto characterCount = countDecodedCharacters(source);
    const auto appendedLength = repeatedCharacterCount(characterCount, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendedLength.toSizeTOrThrow());
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = ByteIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf8::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const U16StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendedLength.toSizeTOrThrow());
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    auto position = U16Index::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf16::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const U16StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto characterCount = countDecodedCharacters(source);
    const auto appendedLength = repeatedCharacterCount(characterCount, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendedLength.toSizeTOrThrow());
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = U16Index::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf16::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const Char character) -> CpCount {
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf32::encodedLength(character).toSizeTOrThrow();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return CpCount::one();
}

auto U32StringAppendTools::append(const Char character, CpCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf32::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U32StringSharedStorage::checkedMultiplySize(characterSize, countSize);
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return count;
}

auto U32StringAppendTools::append(const U8String &text) -> CpCount {
    return append(UnsafeU8StringAccess{text}.dataView());
}

auto U32StringAppendTools::append(const U16String &text) -> CpCount {
    return append(UnsafeU16StringAccess{text}.dataView());
}

auto U32StringAppendTools::append(const U32String &text) -> CpCount {
    return append(UnsafeU32StringAccess{text}.dataView());
}

}
