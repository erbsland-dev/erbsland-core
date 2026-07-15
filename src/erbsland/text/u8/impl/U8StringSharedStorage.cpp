// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringSharedStorage.hpp"

#include "U8StringReadTools.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../mem/impl/SharedArrayCapacity.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::text::impl {

U8StringSharedStorage::U8StringSharedStorage(const std::string_view text) :
    _data{createU8StringData(text)}, _range{unit::ByteRange::fromSizeT(text.size())} {
}

U8StringSharedStorage::U8StringSharedStorage(const std::u8string_view text) :
    _data{createU8StringData(text)}, _range{unit::ByteRange::fromSizeT(text.size())} {
}

U8StringSharedStorage::U8StringSharedStorage(const U8StringLiteralStorage &literal) :
    U8StringSharedStorage{literal.dataView()} {
}

U8StringSharedStorage::U8StringSharedStorage(const U8StringDataView &view) :
    U8StringSharedStorage{U8StringReadTools{view}.toStdString()} {
}

U8StringSharedStorage::U8StringSharedStorage(U8StringDataPtr data, unit::ByteRange range) noexcept :
    _data{std::move(data)}, _range{range} {
}

U8StringSharedStorage::U8StringSharedStorage(const unit::ByteRange range) noexcept :
    _data{createU8StringData(range.length().toSizeT())}, _range{range} {
}

auto U8StringSharedStorage::fromBytes(const std::span<const char> bytes) -> U8StringSharedStorage {
    if (bytes.empty()) {
        return {};
    }
    auto storage = U8StringSharedStorage{unit::ByteRange::fromSizeT(bytes.size())};
    std::memcpy(storage.dataForWrite(), bytes.data(), bytes.size());
    storage.dataForWrite()[bytes.size()] = '\0';
    return storage;
}

auto U8StringSharedStorage::forSize(const std::size_t size) -> U8StringSharedStorage {
    if (size == 0U) {
        return {};
    }
    return U8StringSharedStorage{unit::ByteRange::fromSizeT(size)};
}

void U8StringSharedStorage::validateSize(const std::size_t size) {
    static_cast<void>(unit::ByteLength::fromSizeTOrThrow(size));
}

auto U8StringSharedStorage::checkedAddSize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willAddOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingAdd(first, second);
    validateSize(result);
    return result;
}

auto U8StringSharedStorage::checkedMultiplySize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willMultiplyOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingMultiply(first, second);
    validateSize(result);
    return result;
}

void U8StringSharedStorage::clear() noexcept {
    if (_data.isNull()) {
        _range = unit::ByteRange::empty();
        return;
    }
    const auto reservedCapacity = capacity().toSizeT();
    _data = createU8StringData(0U, reservedCapacity);
    _range = unit::ByteRange::empty();
}

void U8StringSharedStorage::ensureMutableCapacity(const std::size_t requiredCapacity) {
    const auto usedSize = _range.length().toSizeT();
    const auto usedSizeWithTerminator = checkedAddSize(usedSize, 1U, "String storage size exceeds bounds");
    const auto requiredCapacityWithTerminator =
        checkedAddSize(requiredCapacity, 1U, "String storage capacity exceeds bounds");
    const auto oldRange = _range;
    const auto forceReallocate =
        _data.isNull() || _data.isShared() || !_range.index().isZero() || !isFullRange(_data.constGet()->size());
    mem::impl::ensureSharedArrayCapacity(
        _data,
        usedSizeWithTerminator,
        requiredCapacityWithTerminator,
        forceReallocate,
        [usedSize, oldRange](const auto *oldData, auto *newData) {
            if (oldData != nullptr && usedSize > 0U) {
                std::memcpy(newData->data(), oldData->data() + oldRange.index().toSizeT(), usedSize);
            }
            newData->data()[usedSize] = '\0';
        });
    _range = unit::ByteRange::fromSizeT(usedSize);
}

void U8StringSharedStorage::resize(const std::size_t size) noexcept {
    if (_data.isNull()) {
        if (size != 0U) {
            std::terminate();
        }
        _range = unit::ByteRange::empty();
        return;
    }
    if (math::willAddOverflow(size, std::size_t{1U})) {
        std::terminate();
    }
    const auto sizeWithTerminator = math::saturatingAdd(size, std::size_t{1U});
    _data.get()->setSize(static_cast<U8StringData::SizeType>(sizeWithTerminator));
    _data.get()->data()[size] = '\0';
    _range = unit::ByteRange::fromSizeT(size);
}

void U8StringSharedStorage::reserve(const unit::ByteLength capacity) {
    const auto requestedCapacity = capacity.toSizeT();
    if (requestedCapacity <= this->capacity().toSizeT()) {
        return;
    }
    rematerialize(requestedCapacity);
}

void U8StringSharedStorage::shrinkToFit() {
    if (_range.isEmpty()) {
        _data.reset();
        _range = unit::ByteRange::empty();
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

auto U8StringSharedStorage::capacity() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    const auto usableCapacity = static_cast<std::size_t>(_data.constGet()->capacity()) - 1U;
    if (_range.isEmpty()) {
        return unit::ByteLength::fromSizeT(usableCapacity);
    }
    if (isFullRange(_data.constGet()->size())) {
        return unit::ByteLength::fromSizeT(usableCapacity);
    }
    return _range.length();
}

auto U8StringSharedStorage::memoryUsage() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(sizeof(U8StringData) + _data.constGet()->capacity());
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
        _range = unit::ByteRange::fromSizeT(dataSize - 1U);
        return;
    }
    *this = U8StringSharedStorage{dataView()};
}

auto U8StringSharedStorage::dataView() const noexcept -> U8StringDataView {
    if (isEmpty()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U8StringDataView{{data.data(), data.size()}, _range};
}

auto U8StringSharedStorage::dataView(const unit::ByteRange range) const noexcept -> U8StringDataView {
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
    auto data = createU8StringData(usedSize, reservedCapacity);
    if (!_data.isNull() && usedSize > 0U) {
        std::memcpy(data.get()->data(), this->data() + _range.index().toSizeT(), usedSize);
        data.get()->data()[usedSize] = '\0';
    }
    _data = std::move(data);
    _range = unit::ByteRange::fromSizeT(usedSize);
}

}
