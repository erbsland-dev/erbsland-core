// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringAppendTools.hpp"

#include "U32Encoding.hpp"
#include "U32Writer.hpp"

#include "../../impl/RepeatCount.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

auto U32StringAppendTools::append(const U32StringDataView &text) -> U32StringSharedStorage & {
    const auto source = text.dataSpan();
    if (source.empty()) {
        return _storage;
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize, source.size(), "StringEditor append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    std::memcpy(_storage.dataForWrite() + oldSize, source.data(), source.size() * sizeof(char32_t));
    _storage.resize(newSize);
    return _storage;
}

auto U32StringAppendTools::append(const U32StringDataView &text, const unit::ElementCount count)
    -> U32StringSharedStorage & {
    if (count.isZero()) {
        return _storage;
    }
    const auto countSize = repeatCountToSize(count);
    const auto source = text.dataSpan();
    if (source.empty()) {
        return _storage;
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
    return _storage;
}

auto U32StringAppendTools::append(const Char character) -> U32StringSharedStorage & {
    if (!character.isValidUnicode()) {
        return _storage;
    }
    const auto oldSize = _storage.range().length().toSizeT();
    const auto appendSize = impl::utf32::encodedLength(character).toSizeTOrThrow();
    const auto newSize =
        U32StringSharedStorage::checkedAddSize(oldSize, appendSize, "Character append exceeds string size bounds");
    _storage.ensureMutableCapacity(newSize);
    U32Writer writer{std::span<char32_t>{_storage.dataForWrite() + oldSize, appendSize}};
    writer.write(character);
    _storage.resize(newSize);
    return _storage;
}

auto U32StringAppendTools::append(const Char character, unit::CpLength count) -> U32StringSharedStorage & {
    if (count.isZero()) {
        return _storage;
    }
    const auto countSize = repeatCountToSize(count);
    if (!character.isValidUnicode()) {
        return _storage;
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
    return _storage;
}

}
