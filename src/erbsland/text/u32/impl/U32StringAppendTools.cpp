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

auto U32StringAppendTools::append(const U32StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, source.size(), "StringEditor append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size() * sizeof(char32_t));
    _storage.resize(newSize);
    return unit::CpLength::fromSizeT(source.size());
}

auto U32StringAppendTools::append(const U32StringDataView &text, const unit::ItemCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto appendSize = U32StringSharedStorage::checkedMultiplySize(
        source.size(), countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size() * sizeof(char32_t));
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(CpLength::fromSizeTOrThrow(source.size()), countSize);
}

auto U32StringAppendTools::append(const U8StringDataView &text) -> CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendedLength.toSizeTOrThrow(), "StringEditor append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf8::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const U8StringDataView &text, const ItemCount count) -> CpLength {
    if (count.isZero()) {
        return CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpLength::zero();
    }
    const auto characterCount = countDecodedCharacters(source);
    const auto appendedLength = repeatedCharacterCount(characterCount, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendedLength.toSizeTOrThrow(), "Repeated string append exceeds string size bounds");
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

auto U32StringAppendTools::append(const U16StringDataView &text) -> CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendedLength.toSizeTOrThrow(), "StringEditor append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    auto position = U16DataIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf16::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const U16StringDataView &text, const ItemCount count) -> CpLength {
    if (count.isZero()) {
        return CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpLength::zero();
    }
    const auto characterCount = countDecodedCharacters(source);
    const auto appendedLength = repeatedCharacterCount(characterCount, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendedLength.toSizeTOrThrow(), "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendedLength.toSizeTOrThrow()}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = U16DataIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf16::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return appendedLength;
}

auto U32StringAppendTools::append(const Char character) -> unit::CpLength {
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf32::encodedLength(character).toSizeTOrThrow();
    const auto newSize =
        U32StringSharedStorage::checkedAddSize(oldSize, appendSize, "Character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return unit::CpLength::one();
}

auto U32StringAppendTools::append(const Char character, unit::CpLength count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf32::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U32StringSharedStorage::checkedMultiplySize(
        characterSize, countSize, "Repeated character append exceeds string size bounds");
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return count;
}

auto U32StringAppendTools::append(const U8String &text) -> unit::CpLength {
    return append(UnsafeU8StringAccess{text}.dataView());
}

auto U32StringAppendTools::append(const U16String &text) -> unit::CpLength {
    return append(UnsafeU16StringAccess{text}.dataView());
}

auto U32StringAppendTools::append(const U32String &text) -> unit::CpLength {
    return append(UnsafeU32StringAccess{text}.dataView());
}

}
