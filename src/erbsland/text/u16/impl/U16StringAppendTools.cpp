// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringAppendTools.hpp"

#include "U16Encoding.hpp"
#include "U16Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32StringDataView.hpp"
#include "../../u8/impl/U8Encoding.hpp"
#include "../../u8/impl/U8StringDataView.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

auto U16StringAppendTools::repeatedCharacterCount(const unit::CpLength characterCount, const std::size_t countSize)
    -> unit::CpLength {
    const auto totalCharacterCount = U16StringSharedStorage::checkedMultiplySize(
        characterCount.toSizeTOrThrow(), countSize, "Repeated string exceeds character length bounds");
    return unit::CpLength::fromSizeTOrThrow(totalCharacterCount);
}

auto U16StringAppendTools::countDecodedCharacters(const std::span<const char16_t> source) noexcept -> unit::CpLength {
    auto position = unit::U16DataIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < source.size()) {
        utf16::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto U16StringAppendTools::summarizeForUtf16(const std::span<const char> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf8::decodeCharOrReplace(source, position);
        result.encodedLength = U16StringSharedStorage::checkedAddSize(
            result.encodedLength,
            utf16::encodedLength(character).toSizeTOrThrow(),
            "String append exceeds string size bounds");
        ++result.characterCount;
    }
    return result;
}

auto U16StringAppendTools::summarizeForUtf16(const std::span<const char32_t> source) -> AppendSummary {
    auto result = AppendSummary{};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        const auto character = utf32::decodeCharOrReplace(source, position);
        result.encodedLength = U16StringSharedStorage::checkedAddSize(
            result.encodedLength,
            utf16::encodedLength(character).toSizeTOrThrow(),
            "String append exceeds string size bounds");
        ++result.characterCount;
    }
    return result;
}

auto U16StringAppendTools::append(const U16StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize =
        U16StringSharedStorage::checkedAddSize(oldSize, source.size(), "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size() * sizeof(char16_t));
    _storage.resize(newSize);
    return appendedLength;
}

auto U16StringAppendTools::append(const U16StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto appendedLength = countDecodedCharacters(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(
        source.size(), countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    auto *writePosition = _storage.dataForWrite() + oldSize;
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        std::memcpy(writePosition, source.data(), source.size() * sizeof(char16_t));
        writePosition += source.size();
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(appendedLength, countSize);
}

auto U16StringAppendTools::append(const U8StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, summary.encodedLength, "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf8::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U16StringAppendTools::append(const U8StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(
        summary.encodedLength, countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = unit::ByteIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf8::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U16StringAppendTools::append(const U32StringDataView &text) -> unit::CpLength {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, summary.encodedLength, "String append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, summary.encodedLength}};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < source.size()) {
        writer.write(utf32::decodeCharOrReplace(source, position));
    }
    _storage.resize(newSize);
    return summary.characterCount;
}

auto U16StringAppendTools::append(const U32StringDataView &text, const unit::ElementCount count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return unit::CpLength::zero();
    }
    const auto summary = summarizeForUtf16(source);
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(
        summary.encodedLength, countSize, "Repeated string append exceeds string size bounds");
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated string append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        auto position = unit::CpIndex::zero();
        while (position.toSizeT() < source.size()) {
            writer.write(utf32::decodeCharOrReplace(source, position));
        }
    }
    _storage.resize(newSize);
    return repeatedCharacterCount(summary.characterCount, countSize);
}

auto U16StringAppendTools::append(const Char character) -> unit::CpLength {
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf16::encodedLength(character).toSizeTOrThrow();
    const auto newSize =
        U16StringSharedStorage::checkedAddSize(oldSize, appendSize, "Character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return unit::CpLength::one();
}

auto U16StringAppendTools::append(const Char character, unit::CpLength count) -> unit::CpLength {
    if (count.isZero()) {
        return unit::CpLength::zero();
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return unit::CpLength::zero();
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto characterSize = impl::utf16::encodedLength(character).toSizeTOrThrow();
    const auto appendSize = U16StringSharedStorage::checkedMultiplySize(
        characterSize, countSize, "Repeated character append exceeds string size bounds");
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize, appendSize, "Repeated character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U16Writer writer{std::span<char16_t>{_storage.dataForWrite() + oldSize, appendSize}};
    for (auto i = std::size_t{0}; i < countSize; ++i) {
        writer.write(character);
    }
    _storage.resize(newSize);
    return unit::CpLength::fromSizeTOrThrow(countSize);
}

}
