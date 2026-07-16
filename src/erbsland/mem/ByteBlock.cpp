// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlock.hpp"

#include "ByteBlockView.hpp"

#include "impl/ByteBlockData.hpp"
#include "impl/SharedArrayCapacity.hpp"

#include "../err/OutOfRangeError.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace erbsland::mem {

ByteBlock::ByteBlock() = default;

ByteBlock::~ByteBlock() = default;

ByteBlock::ByteBlock(const ByteBlock &) = default;

ByteBlock::ByteBlock(ByteBlock &&) noexcept = default;

auto ByteBlock::operator=(const ByteBlock &) -> ByteBlock & = default;

auto ByteBlock::operator=(ByteBlock &&) noexcept -> ByteBlock & = default;

ByteBlock::ByteBlock(const unit::ByteLength length, const Byte value) :
    _data{createData(length.toSizeTOrThrow(), length.toSizeTOrThrow())} {
    auto *data = dataForWrite();
    std::fill_n(data->data(), data->size(), value);
}

ByteBlock::ByteBlock(const std::span<const Byte> bytes) : _data{createData(bytes)} {
}

ByteBlock::ByteBlock(const std::span<const uint8_t> bytes) : _data{createData(bytes.size(), bytes.size())} {
    auto *data = dataForWrite();
    for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
        data->data()[i] = Byte{bytes[i]};
    }
}

ByteBlock::ByteBlock(const std::span<const char> bytes) : _data{createData(bytes.size(), bytes.size())} {
    auto *data = dataForWrite();
    for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
        data->data()[i] = Byte{static_cast<uint8_t>(bytes[i])};
    }
}

ByteBlock::ByteBlock(const std::vector<Byte> &bytes) : ByteBlock{std::span<const Byte>{bytes}} {
}

ByteBlock::ByteBlock(const std::vector<uint8_t> &bytes) : ByteBlock{std::span<const uint8_t>{bytes}} {
}

ByteBlock::ByteBlock(const std::vector<char> &bytes) : ByteBlock{std::span<const char>{bytes}} {
}

ByteBlock::ByteBlock(impl::ByteBlockDataPtr data) noexcept : _data{std::move(data)} {
}

auto ByteBlock::operator<=>(const ByteBlockView &other) const noexcept -> std::strong_ordering {
    return ByteBlockView{*this} <=> other;
}

auto ByteBlock::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    return ByteBlockView{*this} <=> ByteBlockView{other};
}

auto ByteBlock::startsWith(const ByteBlockView &other) const noexcept -> bool {
    return ByteBlockView{*this}.startsWith(other);
}

auto ByteBlock::endsWith(const ByteBlockView &other) const noexcept -> bool {
    return ByteBlockView{*this}.endsWith(other);
}

auto ByteBlock::contains(const ByteBlockView &other) const noexcept -> bool {
    return ByteBlockView{*this}.contains(other);
}

auto ByteBlock::length() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(_data.constGet()->size());
}

auto ByteBlock::get(const unit::ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return ByteBlockView{*this}.get(index, defaultValue);
}

auto ByteBlock::getOrThrow(const unit::ByteIndex index) const -> Byte {
    return ByteBlockView{*this}.getOrThrow(index);
}

void ByteBlock::set(const unit::ByteIndex index, const Byte value) {
    if (index.isNoIndex() || index.toSizeT() >= length().toSizeT()) {
        return;
    }
    ensureUnique();
    dataForWrite()->data()[index.toSizeT()] = value;
}

void ByteBlock::setOrThrow(const unit::ByteIndex index, const Byte value) {
    if (index.isNoIndex() || index.toSizeT() >= length().toSizeT()) {
        throw err::OutOfRangeError{"Write position out of range"};
    }
    set(index, value);
}

auto ByteBlock::slice(const unit::ByteRange range) const noexcept -> ByteBlockView {
    return ByteBlockView{*this}.slice(range);
}

auto ByteBlock::slice(const unit::ByteIndex begin, const unit::ByteIndex end) const noexcept -> ByteBlockView {
    return slice(unit::ByteRange{begin, end});
}

auto ByteBlock::slice(const unit::ByteIndex begin, const unit::ByteLength length) const noexcept -> ByteBlockView {
    return slice(unit::ByteRange{begin, length});
}

auto ByteBlock::find(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex {
    return ByteBlockView{*this}.find(bytes);
}

auto ByteBlock::find(const ByteBlockView &bytes, const unit::ByteIndex start) const noexcept -> unit::ByteIndex {
    return ByteBlockView{*this}.find(bytes, start);
}

auto ByteBlock::findLast(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex {
    return ByteBlockView{*this}.findLast(bytes);
}

auto ByteBlock::clear() noexcept -> ByteBlock & {
    if (!_data.isNull()) {
        if (_data.isShared()) {
            _data = createData(0U, _data.constGet()->capacity());
        } else {
            _data.get()->setSize(0U);
        }
    }
    return *this;
}

void ByteBlock::reset() noexcept {
    _data.reset();
}

auto ByteBlock::remove(const unit::ByteRange range) -> ByteBlock & {
    const auto clampedRange = range.clampedTo(length());
    if (clampedRange.isEmpty()) {
        return *this;
    }
    ensureUnique();
    auto *data = dataForWrite();
    const auto start = clampedRange.index().toSizeT();
    const auto count = clampedRange.length().toSizeT();
    const auto oldLength = static_cast<std::size_t>(data->size());
    const auto tailStart = start + count;
    const auto tailLength = oldLength - tailStart;
    std::memmove(data->data() + start, data->data() + tailStart, tailLength * sizeof(Byte));
    data->setSize(static_cast<impl::ByteBlockData::SizeType>(oldLength - count));
    return *this;
}

auto ByteBlock::keep(const unit::ByteRange range) -> ByteBlock & {
    if (!range.isValid()) {
        return clear();
    }
    const auto clampedRange = range.clampedTo(length());
    if (clampedRange.isEmpty()) {
        return clear();
    }
    ensureUnique();
    auto *data = dataForWrite();
    const auto start = clampedRange.index().toSizeT();
    const auto count = clampedRange.length().toSizeT();
    std::memmove(data->data(), data->data() + start, count * sizeof(Byte));
    data->setSize(static_cast<impl::ByteBlockData::SizeType>(count));
    return *this;
}

auto ByteBlock::replace(const unit::ByteRange range, const ByteBlockView &replacement) -> ByteBlock & {
    if (!range.isValid()) {
        return *this;
    }
    const auto clampedRange = range.clampedTo(length());
    const auto replacementBytes = replacement.toByteVector();
    const auto oldLength = length().toSizeT();
    const auto start = clampedRange.index().toSizeT();
    const auto removeLength = clampedRange.length().toSizeT();
    const auto replacementLength = replacementBytes.size();
    const auto newLength = oldLength - removeLength + replacementLength;

    ensureCapacity(newLength);
    auto *data = dataForWrite();
    const auto tailStart = start + removeLength;
    const auto tailLength = oldLength - tailStart;
    if (replacementLength != removeLength && tailLength > 0U) {
        std::memmove(data->data() + start + replacementLength, data->data() + tailStart, tailLength * sizeof(Byte));
    }
    if (!replacementBytes.empty()) {
        std::memcpy(data->data() + start, replacementBytes.data(), replacementBytes.size() * sizeof(Byte));
    }
    data->setSize(static_cast<impl::ByteBlockData::SizeType>(newLength));
    return *this;
}

auto ByteBlock::insert(const unit::ByteIndex index, const ByteBlockView &bytes) -> ByteBlock & {
    if (index.isNoIndex()) {
        return *this;
    }
    const auto insertIndex = std::min(index.toSizeT(), length().toSizeT());
    return replace(unit::ByteRange::emptyAt(unit::ByteIndex::fromSizeT(insertIndex)), bytes);
}

auto ByteBlock::append(const Byte value) -> ByteBlock & {
    const auto oldLength = length().toSizeT();
    ensureCapacity(oldLength + 1U);
    auto *data = dataForWrite();
    data->data()[oldLength] = value;
    data->setSize(static_cast<impl::ByteBlockData::SizeType>(oldLength + 1U));
    return *this;
}

auto ByteBlock::append(const ByteBlockView &bytes) -> ByteBlock & {
    const auto appendBytes = bytes.toByteVector();
    if (appendBytes.empty()) {
        return *this;
    }
    const auto oldLength = length().toSizeT();
    const auto newLength = oldLength + appendBytes.size();
    ensureCapacity(newLength);
    auto *data = dataForWrite();
    std::memcpy(data->data() + oldLength, appendBytes.data(), appendBytes.size() * sizeof(Byte));
    data->setSize(static_cast<impl::ByteBlockData::SizeType>(newLength));
    return *this;
}

auto ByteBlock::removed(const unit::ByteRange range) const -> ByteBlock {
    auto result = *this;
    result.remove(range);
    return result;
}

auto ByteBlock::replaced(const unit::ByteRange range, const ByteBlockView &replacement) const -> ByteBlock {
    auto result = *this;
    result.replace(range, replacement);
    return result;
}

auto ByteBlock::join(const std::vector<ByteBlockView> &parts) const -> ByteBlock {
    if (parts.empty()) {
        return {};
    }
    auto result = ByteBlock{};
    for (auto i = std::size_t{0}; i < parts.size(); ++i) {
        if (i > 0U) {
            result.append(ByteBlockView{*this});
        }
        result.append(parts[i]);
    }
    return result;
}

auto ByteBlock::toByteVector() const -> std::vector<Byte> {
    return ByteBlockView{*this}.toByteVector();
}

auto ByteBlock::toUInt8Vector() const -> std::vector<uint8_t> {
    return ByteBlockView{*this}.toUInt8Vector();
}

auto ByteBlock::toCharVector() const -> std::vector<char> {
    return ByteBlockView{*this}.toCharVector();
}

void ByteBlock::detach() {
    ensureUnique();
}

auto ByteBlock::capacity() const noexcept -> unit::ByteLength {
    if (_data.isNull()) {
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::fromSizeT(_data.constGet()->capacity());
}

void ByteBlock::reserve(const unit::ByteLength capacity) {
    if (capacity <= this->capacity()) {
        return;
    }
    ensureCapacity(capacity.toSizeTOrThrow());
}

void ByteBlock::shrinkToFit() {
    const auto currentLength = length().toSizeT();
    if (currentLength == 0U) {
        _data.reset();
        return;
    }
    if (!_data.isShared() && capacity().toSizeT() == currentLength) {
        return;
    }
    _data = createData(ByteBlockView{*this}.dataSpan());
}

auto ByteBlock::fromJoined(const std::vector<ByteBlockView> &parts) -> ByteBlock {
    auto result = ByteBlock{};
    for (const auto &part : parts) {
        result.append(part);
    }
    return result;
}

void ByteBlock::ensureCapacity(const std::size_t requiredCapacity) {
    const auto oldLength = length().toSizeT();
    impl::ensureSharedArrayCapacity(
        _data, oldLength, requiredCapacity, false, [oldLength](const auto *oldData, auto *newData) {
            if (oldData != nullptr && oldLength > 0U) {
                std::memcpy(newData->data(), oldData->data(), oldLength * sizeof(Byte));
            }
        });
}

void ByteBlock::ensureUnique() {
    if (_data.isNull()) {
        return;
    }
    _data.detach();
}

auto ByteBlock::createData(const std::size_t size, const std::size_t capacity) -> impl::ByteBlockDataPtr {
    if (size == 0U && capacity == 0U) {
        return {};
    }
    return impl::ByteBlockDataPtr{impl::ByteBlockData::create(
        static_cast<impl::ByteBlockData::SizeType>(size), static_cast<impl::ByteBlockData::SizeType>(capacity))};
}

auto ByteBlock::createData(const std::span<const Byte> bytes) -> impl::ByteBlockDataPtr {
    auto data = createData(bytes.size(), bytes.size());
    if (!bytes.empty()) {
        std::memcpy(data.get()->data(), bytes.data(), bytes.size() * sizeof(Byte));
    }
    return data;
}

auto ByteBlock::dataForWrite() -> impl::ByteBlockData * {
    return _data.get();
}

}
