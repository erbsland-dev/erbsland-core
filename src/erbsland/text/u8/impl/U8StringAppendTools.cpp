// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringAppendTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16StringDataView.hpp"
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32StringDataView.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

auto U8StringAppendTools::repeatedCharacterCount(const unit::CpLength characterCount, const std::size_t countSize)
    -> unit::CpLength {
    const auto totalCharacterCount = U8StringSharedStorage::checkedMultiplySize(
        characterCount.toSizeTOrThrow(), countSize, "Repeated string exceeds character length bounds");
    return unit::CpLength::fromSizeTOrThrow(totalCharacterCount);
}

auto U8StringAppendTools::countDecodedCharacters(const std::span<const char> source) noexcept -> unit::CpLength {
    auto position = unit::ByteIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < source.size()) {
        utf8::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto U8StringAppendTools::summarizeForUtf8(const std::span<const char16_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf16::decodeCharOrReplace(source, position);
        result.encodedLength = U8StringSharedStorage::checkedAddSize(
            result.encodedLength,
            utf8::encodedLength(character).toSizeTOrThrow(),
            "String append exceeds string size bounds");
        ++result.characterCount;
    }
    return result;
}

auto U8StringAppendTools::summarizeForUtf8(const std::span<const char32_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf32::decodeCharOrReplace(source, position);
        result.encodedLength = U8StringSharedStorage::checkedAddSize(
            result.encodedLength,
            utf8::encodedLength(character).toSizeTOrThrow(),
            "String append exceeds string size bounds");
        ++result.characterCount;
    }
    return result;
}

auto U8StringAppendTools::append(const U8StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize =
        U8StringSharedStorage::checkedAddSize(oldSize, source.size(), "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size());
    _storage.resize(newSize);
    return appendedLength;
}

auto U8StringAppendTools::append(const U8StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(
        source.size(), countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize =
        U8StringSharedStorage::checkedAddSize(oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size());
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(appendedLength, countSize);
}

auto U8StringAppendTools::append(const U16StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        oldSize, summary.encodedLength, "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf16::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U8StringAppendTools::append(const U16StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(
        summary.encodedLength, countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize =
        U8StringSharedStorage::checkedAddSize(oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = unit::U16DataIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf16::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U8StringAppendTools::append(const U32StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        oldSize, summary.encodedLength, "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf32::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U8StringAppendTools::append(const U32StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf8(source);
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(
        summary.encodedLength, countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize =
        U8StringSharedStorage::checkedAddSize(oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = unit::CpIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf32::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U8StringAppendTools::append(const Char character) -> unit::CpLength {
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf8::encodedLength(character).toSizeTOrThrow();
    const auto newSize =
        U8StringSharedStorage::checkedAddSize(oldSize, appendSize, "Character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return unit::CpLength::one();
}

auto U8StringAppendTools::append(const Char character, unit::CpLength count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf8::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U8StringSharedStorage::checkedMultiplySize(
        characterSize, countSize, "Repeated character append exceeds string size bounds");
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U8Writer writer{std::span<char>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return unit::CpLength::fromSizeTOrThrow(countSize);
}

}
