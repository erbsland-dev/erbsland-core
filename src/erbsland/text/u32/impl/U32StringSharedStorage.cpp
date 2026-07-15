// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringSharedStorage.hpp"

#include "U32StringReadTools.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../mem/impl/SharedArrayCapacity.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::text::impl {

U32StringSharedStorage::U32StringSharedStorage(const std::u32string_view text) :
    _data{createU32StringData(text)}, _range{unit::CpRange::fromSizeT(text.size())} {
}

U32StringSharedStorage::U32StringSharedStorage(const U32StringLiteralStorage &literal) :
    U32StringSharedStorage{literal.dataView()} {
}

U32StringSharedStorage::U32StringSharedStorage(const U32StringDataView &view) :
    U32StringSharedStorage{U32StringReadTools{view}.toStdU32String()} {
}

U32StringSharedStorage::U32StringSharedStorage(U32StringDataPtr data, unit::CpRange range) noexcept :
    _data{std::move(data)}, _range{range} {
}

U32StringSharedStorage::U32StringSharedStorage(const unit::CpRange range) noexcept :
    _data{createU32StringData(range.length().toSizeT())}, _range{range} {
}

auto U32StringSharedStorage::fromCodeUnits(const std::span<const char32_t> codeUnits) -> U32StringSharedStorage {
    if (codeUnits.empty()) {
        return {};
    }
    auto storage = U32StringSharedStorage{unit::CpRange::fromSizeT(codeUnits.size())};
    std::memcpy(storage.dataForWrite(), codeUnits.data(), codeUnits.size() * sizeof(char32_t));
    storage.dataForWrite()[codeUnits.size()] = U'\0';
    return storage;
}

auto U32StringSharedStorage::forSize(const std::size_t size) -> U32StringSharedStorage {
    if (size == 0U) {
        return {};
    }
    return U32StringSharedStorage{unit::CpRange::fromSizeT(size)};
}

void U32StringSharedStorage::validateSize(const std::size_t size) {
    static_cast<void>(unit::CpLength::fromSizeTOrThrow(size));
}

auto U32StringSharedStorage::checkedAddSize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willAddOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingAdd(first, second);
    validateSize(result);
    return result;
}

auto U32StringSharedStorage::checkedMultiplySize(
    const std::size_t first, const std::size_t second, const std::string_view reason) -> std::size_t {
    if (math::willMultiplyOverflow(first, second)) {
        text::impl::throwOverflow(reason);
    }
    const auto result = math::saturatingMultiply(first, second);
    validateSize(result);
    return result;
}

void U32StringSharedStorage::clear() noexcept {
    if (_data.isNull()) {
        _range = unit::CpRange::empty();
        return;
    }
    const auto reservedCapacity = capacity().toSizeT();
    _data = createU32StringData(0U, reservedCapacity);
    _range = unit::CpRange::empty();
}

void U32StringSharedStorage::ensureMutableCapacity(const std::size_t requiredCapacity) {
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
                std::memcpy(newData->data(), oldData->data() + oldRange.index().toSizeT(), usedSize * sizeof(char32_t));
            }
            newData->data()[usedSize] = U'\0';
        });
    _range = unit::CpRange::fromSizeT(usedSize);
}

void U32StringSharedStorage::resize(const std::size_t size) noexcept {
    if (_data.isNull()) {
        if (size != 0U) {
            std::terminate();
        }
        _range = unit::CpRange::empty();
        return;
    }
    if (math::willAddOverflow(size, std::size_t{1U})) {
        std::terminate();
    }
    const auto sizeWithTerminator = math::saturatingAdd(size, std::size_t{1U});
    _data.get()->setSize(static_cast<U32StringData::SizeType>(sizeWithTerminator));
    _data.get()->data()[size] = U'\0';
    _range = unit::CpRange::fromSizeT(size);
}

void U32StringSharedStorage::reserve(const unit::CpLength capacity) {
    const auto requestedCapacity = capacity.toSizeT();
    if (requestedCapacity <= this->capacity().toSizeT()) {
        return;
    }
    rematerialize(requestedCapacity);
}

void U32StringSharedStorage::shrinkToFit() {
    if (_range.isEmpty()) {
        _data.reset();
        _range = unit::CpRange::empty();
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

auto U32StringSharedStorage::capacity() const noexcept -> unit::CpLength {
    if (_data.isNull()) {
        return unit::CpLength::zero();
    }
    const auto usableCapacity = static_cast<std::size_t>(_data.constGet()->capacity()) - 1U;
    if (_range.isEmpty()) {
        return unit::CpLength::fromSizeT(usableCapacity);
    }
    if (isFullRange(_data.constGet()->size())) {
        return unit::CpLength::fromSizeT(usableCapacity);
    }
    return _range.length();
}

auto U32StringSharedStorage::memoryUsage() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(sizeof(U32StringData) + _data.constGet()->capacity() * sizeof(char32_t));
}

void U32StringSharedStorage::detach() {
    if (isEmpty()) {
        return;
    }
    const auto dataSize = _data.constGet()->size();
    if (isFullRange(dataSize) && !_data.isShared()) {
        return;
    }
    if (isFullRange(dataSize)) {
        _data.detach();
        _range = unit::CpRange::fromSizeT(dataSize - 1U);
        return;
    }
    *this = U32StringSharedStorage{dataView()};
}

auto U32StringSharedStorage::dataView() const noexcept -> U32StringDataView {
    if (isEmpty()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U32StringDataView{{data.data(), data.size()}, _range};
}

auto U32StringSharedStorage::dataView(const unit::CpRange range) const noexcept -> U32StringDataView {
    if (_data.isNull()) {
        return {};
    }
    const auto &data = *_data.constGet();
    return U32StringDataView{{data.data(), data.size()}, range};
}

auto U32StringSharedStorage::isFullRange(const std::size_t dataSize) const noexcept -> bool {
    return dataSize > 0U && _range.index().isZero() && _range.length().toSizeT() == dataSize - 1U;
}

void U32StringSharedStorage::rematerialize(const std::size_t reservedCapacity) {
    const auto usedSize = _range.length().toSizeT();
    auto data = createU32StringData(usedSize, reservedCapacity);
    if (!_data.isNull() && usedSize > 0U) {
        std::memcpy(data.get()->data(), this->data() + _range.index().toSizeT(), usedSize * sizeof(char32_t));
        data.get()->data()[usedSize] = U'\0';
    }
    _data = std::move(data);
    _range = unit::CpRange::fromSizeT(usedSize);
}

}
