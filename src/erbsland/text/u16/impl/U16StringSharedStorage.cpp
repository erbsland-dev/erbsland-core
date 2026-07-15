// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringSharedStorage.hpp"

#include "U16StringReadTools.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../mem/impl/SharedArrayCapacity.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::text::impl {

U16StringSharedStorage::U16StringSharedStorage(const std::u16string_view text) :
    _data{createU16StringData(text)}, _range{unit::U16DataRange::fromSizeT(text.size())} {
}

U16StringSharedStorage::U16StringSharedStorage(const U16StringLiteralStorage &literal) :
    U16StringSharedStorage{literal.dataView()} {
}

U16StringSharedStorage::U16StringSharedStorage(const U16StringDataView &view) :
    U16StringSharedStorage{U16StringReadTools{view}.toStdU16String()} {
}

U16StringSharedStorage::U16StringSharedStorage(U16StringDataPtr data, unit::U16DataRange range) noexcept :
    _data{std::move(data)}, _range{range} {
}

U16StringSharedStorage::U16StringSharedStorage(const unit::U16DataRange range) noexcept :
    _data{createU16StringData(range.length().toSizeT())}, _range{range} {
}

auto U16StringSharedStorage::fromCodeUnits(const std::span<const char16_t> codeUnits) -> U16StringSharedStorage {
    if (codeUnits.empty()) {
        return {};
    }
    auto storage = U16StringSharedStorage{unit::U16DataRange::fromSizeT(codeUnits.size())};
    std::memcpy(storage.dataForWrite(), codeUnits.data(), codeUnits.size() * sizeof(char16_t));
    storage.dataForWrite()[codeUnits.size()] = u'\0';
    return storage;
}

auto U16StringSharedStorage::forSize(const std::size_t size) -> U16StringSharedStorage {
    if (size == 0U) {
        return {};
    }
    return U16StringSharedStorage{unit::U16DataRange::fromSizeT(size)};
}

void U16StringSharedStorage::validateSize(const std::size_t size) {
    static_cast<void>(unit::U16DataLength::fromSizeTOrThrow(size));
}

auto U16StringSharedStorage::checkedAddSize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willAddOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingAdd(first, second);
    validateSize(result);
    return result;
}

auto U16StringSharedStorage::checkedMultiplySize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willMultiplyOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingMultiply(first, second);
    validateSize(result);
    return result;
}

void U16StringSharedStorage::clear() noexcept {
    if (_data.isNull()) {
        _range = unit::U16DataRange::empty();
        return;
    }
    const auto reservedCapacity = capacity().toSizeT();
    _data = createU16StringData(0U, reservedCapacity);
    _range = unit::U16DataRange::empty();
}

void U16StringSharedStorage::ensureMutableCapacity(const std::size_t requiredCapacity) {
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
                std::memcpy(newData->data(), oldData->data() + oldRange.index().toSizeT(), usedSize * sizeof(char16_t));
            }
            newData->data()[usedSize] = u'\0';
        });
    _range = unit::U16DataRange::fromSizeT(usedSize);
}

void U16StringSharedStorage::resize(const std::size_t size) noexcept {
    if (_data.isNull()) {
        if (size != 0U) {
            std::terminate();
        }
        _range = unit::U16DataRange::empty();
        return;
    }
    if (math::willAddOverflow(size, std::size_t{1U})) {
        std::terminate();
    }
    const auto sizeWithTerminator = math::saturatingAdd(size, std::size_t{1U});
    _data.get()->setSize(static_cast<U16StringData::SizeType>(sizeWithTerminator));
    _data.get()->data()[size] = u'\0';
    _range = unit::U16DataRange::fromSizeT(size);
}

void U16StringSharedStorage::reserve(const unit::U16DataLength capacity) {
    const auto requestedCapacity = capacity.toSizeT();
    if (requestedCapacity <= this->capacity().toSizeT()) {
        return;
    }
    rematerialize(requestedCapacity);
}

void U16StringSharedStorage::shrinkToFit() {
    if (_range.isEmpty()) {
        _data.reset();
        _range = unit::U16DataRange::empty();
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

auto U16StringSharedStorage::capacity() const noexcept -> unit::U16DataLength {
    if (_data.isNull()) {
        return unit::U16DataLength::zero();
    }
    const auto usableCapacity = static_cast<std::size_t>(_data.constGet()->capacity()) - 1U;
    if (_range.isEmpty()) {
        return unit::U16DataLength::fromSizeT(usableCapacity);
    }
    if (isFullRange(_data.constGet()->size())) {
        return unit::U16DataLength::fromSizeT(usableCapacity);
    }
    return _range.length();
}

auto U16StringSharedStorage::memoryUsage() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(sizeof(U16StringData) + _data.constGet()->capacity() * sizeof(char16_t));
}

void U16StringSharedStorage::detach() {
    if (isEmpty()) {
        return;
    }
    const auto dataSize = _data.constGet()->size();
    if (isFullRange(dataSize) && !_data.isShared()) {
        return;
    }
    if (isFullRange(dataSize)) {
        _data.detach();
        _range = unit::U16DataRange::fromSizeT(dataSize - 1U);
        return;
    }
    *this = U16StringSharedStorage{dataView()};
}

auto U16StringSharedStorage::dataView() const noexcept -> U16StringDataView {
    if (isEmpty()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U16StringDataView{{data.data(), data.size()}, _range};
}

auto U16StringSharedStorage::dataView(const unit::U16DataRange range) const noexcept -> U16StringDataView {
    if (_data.isNull()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U16StringDataView{{data.data(), data.size()}, range};
}

auto U16StringSharedStorage::isFullRange(const std::size_t dataSize) const noexcept -> bool {
    return dataSize > 0U && _range.index().isZero() && _range.length().toSizeT() == dataSize - 1U;
}

void U16StringSharedStorage::rematerialize(const std::size_t reservedCapacity) {
    const auto usedSize = _range.length().toSizeT();
    auto data = createU16StringData(usedSize, reservedCapacity);
    if (!_data.isNull() && usedSize > 0U) {
        std::memcpy(data.get()->data(), this->data() + _range.index().toSizeT(), usedSize * sizeof(char16_t));
        data.get()->data()[usedSize] = u'\0';
    }
    _data = std::move(data);
    _range = unit::U16DataRange::fromSizeT(usedSize);
}

}
