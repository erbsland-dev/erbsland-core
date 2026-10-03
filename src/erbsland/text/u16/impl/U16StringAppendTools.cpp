// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringAppendTools.hpp"

#include "U16Encoding.hpp"
#include "U16Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../impl/UnsafeU16StringAccess.hpp"
#include "../../impl/UnsafeU32StringAccess.hpp"
#include "../../impl/UnsafeU8StringAccess.hpp"
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32StringDataView.hpp"
#include "../../u8/impl/U8Encoding.hpp"
#include "../../u8/impl/U8StringDataView.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

using namespace unit;

auto U16StringAppendTools::summarizeForUtf16(const std::span<const char> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf8::decodeCharOrReplace(source, position);
        result.encodedLength = U16StringSharedStorage::checkedAddSize(
            result.encodedLength, utf16::encodedLength(character).toSizeTOrThrow());
        ++result.characterCount;
    }
    return result;
}

auto U16StringAppendTools::summarizeForUtf16(const std::span<const char32_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf32::decodeCharOrReplace(source, position);
        result.encodedLength = U16StringSharedStorage::checkedAddSize(
            result.encodedLength, utf16::encodedLength(character).toSizeTOrThrow());
        ++result.characterCount;
    }
    return result;
}

auto U16StringAppendTools::append(const U16StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, source.size());
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size() * sizeof(char16_t));
    _storage.resize(newSize);
    return appendedLength;
}

auto U16StringAppendTools::append(const U16StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(source.size(), countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size() * sizeof(char16_t));
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(appendedLength, countSize);
}

auto U16StringAppendTools::append(const U8StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, summary.encodedLength);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf8::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U16StringAppendTools::append(const U8StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(summary.encodedLength, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = ByteIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf8::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U16StringAppendTools::append(const U32StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, summary.encodedLength);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf32::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U16StringAppendTools::append(const U32StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(summary.encodedLength, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = CpIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf32::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U16StringAppendTools::append(const Char character) -> CpCount {
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf16::encodedLength(character).toSizeTOrThrow();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return CpCount::one();
}

auto U16StringAppendTools::append(const Char character, CpCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf16::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(characterSize, countSize);
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return CpCount::fromSizeTOrThrow(countSize);
}

auto U16StringAppendTools::append(const U8String &text) -> unit::CpCount {
    return append(UnsafeU8StringAccess{text}.dataView());
}

auto U16StringAppendTools::append(const U16String &text) -> unit::CpCount {
    return append(UnsafeU16StringAccess{text}.dataView());
}

auto U16StringAppendTools::append(const U32String &text) -> unit::CpCount {
    return append(UnsafeU32StringAccess{text}.dataView());
}

}
