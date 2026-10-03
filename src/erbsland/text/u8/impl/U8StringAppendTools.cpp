// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringAppendTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../impl/UnsafeU16StringAccess.hpp"
#include "../../impl/UnsafeU32StringAccess.hpp"
#include "../../impl/UnsafeU8StringAccess.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16StringDataView.hpp"
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32StringDataView.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

using namespace unit;

auto U8StringAppendTools::summarizeForUtf8(const std::span<const char16_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = U16Index::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf16::decodeCharOrReplace(source, position);
        result.encodedLength = U8StringSharedStorage::checkedAddSize(
            result.encodedLength, utf8::encodedLength(character).toSizeTOrThrow());
        ++result.characterCount;
    }
    return result;
}

auto U8StringAppendTools::summarizeForUtf8(const std::span<const char32_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf32::decodeCharOrReplace(source, position);
        result.encodedLength = U8StringSharedStorage::checkedAddSize(
            result.encodedLength, utf8::encodedLength(character).toSizeTOrThrow());
        ++result.characterCount;
    }
    return result;
}

auto U8StringAppendTools::append(const U8StringDataView &text) -> CpCount {
    return append(text.dataSpan());
}

auto U8StringAppendTools::append(const std::span<const std::byte> text) -> CpCount {
    return append(std::span<const char>{reinterpret_cast<const char *>(text.data()), text.size()});
}

auto U8StringAppendTools::append(const mem::ConstByteSpan text) -> CpCount {
    return append(std::as_bytes(text));
}

auto U8StringAppendTools::append(const std::span<const char> source) -> CpCount {
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, source.size());
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size());
    _storage.resize(newSize);
    return appendedLength;
}

auto U8StringAppendTools::append(const U8StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(source.size(), countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size());
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(appendedLength, countSize);
}

auto U8StringAppendTools::append(const U16StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, summary.encodedLength);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = U16Index::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf16::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U8StringAppendTools::append(const U16StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(summary.encodedLength, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = U16Index::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf16::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U8StringAppendTools::append(const U32StringDataView &text) -> CpCount {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, summary.encodedLength);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf32::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U8StringAppendTools::append(const U32StringDataView &text, const ItemCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return CpCount::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(summary.encodedLength, countSize);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = CpIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf32::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U8StringAppendTools::append(const Char character) -> CpCount {
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf8::encodedLength(character).toSizeTOrThrow();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return CpCount::one();
}

auto U8StringAppendTools::append(const Char character, CpCount count) -> CpCount {
    if (count.isZero()) {
        return CpCount::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return CpCount::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf8::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(characterSize, countSize);
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, appendSize);
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return CpCount::fromSizeTOrThrow(countSize);
}

auto U8StringAppendTools::append(const U8String &text) -> unit::CpCount {
    return append(UnsafeU8StringAccess{text}.dataView());
}

auto U8StringAppendTools::append(const U16String &text) -> unit::CpCount {
    return append(UnsafeU16StringAccess{text}.dataView());
}

auto U8StringAppendTools::append(const U32String &text) -> unit::CpCount {
    return append(UnsafeU32StringAccess{text}.dataView());
}

}
