// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringSharedStorage.hpp"

#include "U8StringData.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../mem/impl/BestGrowth.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::text::impl {

using unit::ByteLength;
using unit::ByteRange;

auto U8StringSharedStorage::isEmpty() const noexcept -> bool {
    return _range.isEmpty() || _data.isNull();
}

U8StringSharedStorage::U8StringSharedStorage(const std::string_view text, const bool sensitive) :
    _data{createU8StringData(text, sensitive)}, _range{ByteRange::fromSizeT(text.size())} {
}

U8StringSharedStorage::U8StringSharedStorage(const std::u8string_view text, const bool sensitive) :
    _data{createU8StringData(text, sensitive)}, _range{ByteRange::fromSizeT(text.size())} {
}

U8StringSharedStorage::U8StringSharedStorage(const U8StringLiteralStorage &literal, const bool sensitive) :
    U8StringSharedStorage{literal.dataView(), sensitive} {
}

U8StringSharedStorage::U8StringSharedStorage(const U8StringDataView &view, const bool sensitive) :
    U8StringSharedStorage{ByteRange::fromSizeT(view.dataSpan().size()), sensitive} {
    if (!view.dataSpan().empty()) {
        std::memcpy(dataForWrite(), view.dataSpan().data(), view.dataSpan().size());
    }
}

U8StringSharedStorage::U8StringSharedStorage(U8StringDataPtr data, ByteRange range) noexcept :
    _data{std::move(data)}, _range{range} {
}

U8StringSharedStorage::U8StringSharedStorage(const ByteRange range, const bool sensitive) noexcept :
    _data{createU8StringData(range.length().toSizeT(), sensitive)}, _range{range} {
}

auto U8StringSharedStorage::isSensitive() const noexcept -> bool {
    return !_data.isNull() && _data.constGet()->isSensitive();
}

void U8StringSharedStorage::markAsSensitive() noexcept {
    if (_data.isNull()) {
        return;
    }
    _data.constGet()->setSensitive();
}

auto U8StringSharedStorage::data() const noexcept -> mem::UnsafeConstCharPtr {
    if (_data.isNull()) {
        return nullptr;
    }
    return _data.constGet()->data();
}

auto U8StringSharedStorage::dataForWrite() noexcept -> mem::UnsafeCharPtr {
    if (_data.isNull()) {
        return nullptr;
    }
    return _data.get()->data();
}

auto U8StringSharedStorage::dataSize() const noexcept -> std::size_t {
    if (_data.isNull()) {
        return 0;
    }
    return _data.get()->size() - 1U;
}

auto U8StringSharedStorage::storageId() const noexcept -> mem::StorageIdentifier {
    if (data() == nullptr) {
        return {};
    }
    const auto *begin = data() + _range.index().toSizeT();
    return mem::StorageIdentifier::fromMemoryRange(begin, begin + _range.length().toSizeT());
}

auto U8StringSharedStorage::isUniqueFullRange() const noexcept -> bool {
    return !_data.isNull() && !_data.isShared() && isFullRange(_data.constGet()->size());
}

auto U8StringSharedStorage::fromBytes(const std::span<const char> bytes, const bool sensitive)
    -> U8StringSharedStorage {
    if (bytes.empty()) {
        return {};
    }
    auto storage = U8StringSharedStorage{ByteRange::fromSizeT(bytes.size()), sensitive};
    std::memcpy(storage.dataForWrite(), bytes.data(), bytes.size());
    storage.dataForWrite()[bytes.size()] = '\0';
    return storage;
}

auto U8StringSharedStorage::forSize(const std::size_t size, const bool sensitive) -> U8StringSharedStorage {
    if (size == 0U) {
        return {};
    }
    return U8StringSharedStorage{ByteRange::fromSizeT(size), sensitive};
}

void U8StringSharedStorage::validateSize(const std::size_t size) {
    if (math::willCastOverflow<ByteLength::Value>(size)) {
        throwOverflow("String storage size exceeds bounds");
    }
}

auto U8StringSharedStorage::checkedAddSize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willAddOverflow(first, second)) {
        throwOverflow(reason);
    }
    const auto result = math::saturatingAdd(first, second);
    validateSize(result);
    return result;
}

auto U8StringSharedStorage::checkedMultiplySize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willMultiplyOverflow(first, second)) {
        throwOverflow(reason);
    }
    const auto result = math::saturatingMultiply(first, second);
    validateSize(result);
    return result;
}

void U8StringSharedStorage::clear() noexcept {
    if (_data.isNull()) {
        _range = ByteRange::empty();
        return;
    }
    const auto reservedCapacity = capacity().toSizeT();
    _data = createU8StringData(0U, reservedCapacity, isSensitive());
    _range = ByteRange::empty();
}

void U8StringSharedStorage::ensureMutableCapacity(const std::size_t requiredCapacity) {
    const auto usedSize = _range.length().toSizeT();
    const auto usedSizeWithTerminator = checkedAddSize(usedSize, 1U, "StringEditor storage size exceeds bounds");
    const auto requiredCapacityWithTerminator =
        checkedAddSize(requiredCapacity, 1U, "StringEditor storage capacity exceeds bounds");
    const auto oldRange = _range;
    const auto oldSensitive = isSensitive();
    const auto forceReallocate =
        _data.isNull() || _data.isShared() || !_range.index().isZero() || !isFullRange(_data.constGet()->size());
    if (forceReallocate || requiredCapacityWithTerminator > _data.constGet()->capacity()) {
        const auto oldCapacity =
            _data.isNull() ? std::size_t{} : static_cast<std::size_t>(_data.constGet()->capacity());
        const auto requestedCapacity = std::max(usedSizeWithTerminator, requiredCapacityWithTerminator);
        const auto newCapacity = mem::impl::BestGrowth{oldCapacity, requestedCapacity}.bestGrowth<U8StringData>();
        auto newData = createU8StringData(usedSize, newCapacity - 1U, oldSensitive);
        if (!_data.isNull() && usedSize > 0U) {
            std::memcpy(newData.get()->data(), _data.constGet()->data() + oldRange.index().toSizeT(), usedSize);
        }
        newData.get()->data()[usedSize] = '\0';
        _data = std::move(newData);
    }
    _range = ByteRange::fromSizeT(usedSize);
}

void U8StringSharedStorage::resize(const std::size_t size) noexcept {
    if (_data.isNull()) {
        if (size != 0U) {
            std::terminate();
        }
        _range = ByteRange::empty();
        return;
    }
    if (math::willAddOverflow(size, std::size_t{1U})) {
        std::terminate();
    }
    const auto sizeWithTerminator = math::saturatingAdd(size, std::size_t{1U});
    _data.get()->setSize(static_cast<U8StringData::SizeType>(sizeWithTerminator));
    _data.get()->data()[size] = '\0';
    _range = ByteRange::fromSizeT(size);
}

void U8StringSharedStorage::reserve(const ByteLength capacity) {
    const auto requestedCapacity = capacity.toSizeT();
    if (requestedCapacity <= this->capacity().toSizeT()) {
        return;
    }
    rematerialize(requestedCapacity);
}

void U8StringSharedStorage::shrinkToFit() {
    if (_range.isEmpty()) {
        _data.reset();
        _range = ByteRange::empty();
        return;
    }
    const auto usedSize = _range.length().toSizeT();
    const auto isExactUniqueFullRange =
        !_data.isNull() && !_data.isShared() && isFullRange(_data.constGet()->size()) && capacity() == _range.length();
    if (isExactUniqueFullRange) {
        return;
    }
    rematerialize(usedSize);
}

auto U8StringSharedStorage::capacity() const noexcept -> ByteLength {
    if (_data.isNull()) {
        return ByteLength::zero();
    }
    const auto usableCapacity = static_cast<std::size_t>(_data.constGet()->capacity()) - 1U;
    if (_range.isEmpty()) {
        return ByteLength::fromSizeT(usableCapacity);
    }
    if (isFullRange(_data.constGet()->size())) {
        return ByteLength::fromSizeT(usableCapacity);
    }
    return _range.length();
}

auto U8StringSharedStorage::memoryUsage() const noexcept -> ByteLength {
    if (_data.isNull()) {
        return ByteLength::zero();
    }
    return ByteLength::fromSizeT(sizeof(U8StringData) + _data.constGet()->capacity());
}

void U8StringSharedStorage::detach() {
    if (isEmpty()) {
        return;
    }
    const auto dataSize = _data.constGet()->size();
    if (isFullRange(dataSize) && !_data.isShared()) {
        return;
    }
    if (isFullRange(dataSize)) {
        _data.detach();
        _range = ByteRange::fromSizeT(dataSize - 1U);
        return;
    }
    *this = U8StringSharedStorage{dataView(), isSensitive()};
}

auto U8StringSharedStorage::dataView() const noexcept -> U8StringDataView {
    if (isEmpty()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U8StringDataView{{data.data(), data.size()}, _range};
}

auto U8StringSharedStorage::dataView(const ByteRange range) const noexcept -> U8StringDataView {
    if (_data.isNull()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U8StringDataView{{data.data(), data.size()}, range};
}

auto U8StringSharedStorage::isFullRange(const std::size_t dataSize) const noexcept -> bool {
    return dataSize > 0U && _range.index().isZero() && _range.length().toSizeT() == dataSize - 1U;
}

void U8StringSharedStorage::rematerialize(const std::size_t reservedCapacity) {
    const auto usedSize = _range.length().toSizeT();
    auto data = createU8StringData(usedSize, reservedCapacity, isSensitive());
    if (!_data.isNull() && usedSize > 0U) {
        std::memcpy(data.get()->data(), this->data() + _range.index().toSizeT(), usedSize);
        data.get()->data()[usedSize] = '\0';
    }
    _data = std::move(data);
    _range = ByteRange::fromSizeT(usedSize);
}

}
