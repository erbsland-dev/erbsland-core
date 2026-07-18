// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockEditor.hpp"

#include "ByteBlock.hpp"

#include "impl/ByteBlockData.hpp"
#include "impl/SharedArrayCapacity.hpp"

#include "../err/OutOfRangeError.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace erbsland::mem {

using impl::ByteBlockData;
using impl::ByteBlockDataPtr;
using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteRange;

ByteBlockEditor::ByteBlockEditor() = default;

ByteBlockEditor::~ByteBlockEditor() = default;

ByteBlockEditor::ByteBlockEditor(const ByteBlockEditor &) = default;

ByteBlockEditor::ByteBlockEditor(ByteBlockEditor &&) noexcept = default;

auto ByteBlockEditor::operator=(const ByteBlockEditor &) -> ByteBlockEditor & = default;

auto ByteBlockEditor::operator=(ByteBlockEditor &&) noexcept -> ByteBlockEditor & = default;

ByteBlockEditor::ByteBlockEditor(const ByteLength length, const Byte value) :
    _data{createData(length.toSizeTOrThrow(), length.toSizeTOrThrow())} {
    auto *data = dataForWrite();
    std::fill_n(data->data(), data->size(), value);
}

ByteBlockEditor::ByteBlockEditor(const std::span<const Byte> bytes) : _data{createData(bytes)} {
}

ByteBlockEditor::ByteBlockEditor(const std::span<const uint8_t> bytes) : _data{createData(bytes.size(), bytes.size())} {
    auto *data = dataForWrite();
    for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
        data->data()[i] = Byte{bytes[i]};
    }
}

ByteBlockEditor::ByteBlockEditor(const std::span<const char> bytes) : _data{createData(bytes.size(), bytes.size())} {
    auto *data = dataForWrite();
    for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
        data->data()[i] = Byte{static_cast<uint8_t>(bytes[i])};
    }
}

ByteBlockEditor::ByteBlockEditor(const std::vector<Byte> &bytes) : ByteBlockEditor{std::span<const Byte>{bytes}} {
}

ByteBlockEditor::ByteBlockEditor(const std::vector<uint8_t> &bytes) : ByteBlockEditor{std::span<const uint8_t>{bytes}} {
}

ByteBlockEditor::ByteBlockEditor(const std::vector<char> &bytes) : ByteBlockEditor{std::span<const char>{bytes}} {
}

ByteBlockEditor::ByteBlockEditor(const ByteBlock &block) : _data{createData(block.dataSpan())} {
}

ByteBlockEditor::ByteBlockEditor(ByteBlockDataPtr data) noexcept : _data{std::move(data)} {
}

auto ByteBlockEditor::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    return ByteBlock{*this} <=> other;
}

auto ByteBlockEditor::operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering {
    return ByteBlock{*this} <=> ByteBlock{other};
}

auto ByteBlockEditor::startsWith(const ByteBlock &other) const noexcept -> bool {
    return ByteBlock{*this}.startsWith(other);
}

auto ByteBlockEditor::endsWith(const ByteBlock &other) const noexcept -> bool {
    return ByteBlock{*this}.endsWith(other);
}

auto ByteBlockEditor::contains(const ByteBlock &other) const noexcept -> bool {
    return ByteBlock{*this}.contains(other);
}

auto ByteBlockEditor::length() const noexcept -> ByteLength {
    if (_data.isNull()) {
        return ByteLength::zero();
    }
    return ByteLength::fromSizeT(_data.constGet()->size());
}

auto ByteBlockEditor::get(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return ByteBlock{*this}.get(index, defaultValue);
}

auto ByteBlockEditor::getOrThrow(const ByteIndex index) const -> Byte {
    return ByteBlock{*this}.getOrThrow(index);
}

void ByteBlockEditor::set(const ByteIndex index, const Byte value) {
    if (index.isNoIndex() || index.toSizeT() >= length().toSizeT()) {
        return;
    }
    ensureUnique();
    dataForWrite()->data()[index.toSizeT()] = value;
}

void ByteBlockEditor::setOrThrow(const ByteIndex index, const Byte value) {
    if (index.isNoIndex() || index.toSizeT() >= length().toSizeT()) {
        throw err::OutOfRangeError{"Write position out of range"};
    }
    set(index, value);
}

auto ByteBlockEditor::slice(const ByteRange range) const noexcept -> ByteBlock {
    return ByteBlock{*this}.slice(range);
}

auto ByteBlockEditor::slice(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, end});
}

auto ByteBlockEditor::slice(const ByteIndex begin, const ByteLength length) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, length});
}

auto ByteBlockEditor::find(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return ByteBlock{*this}.find(bytes);
}

auto ByteBlockEditor::find(const ByteBlock &bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return ByteBlock{*this}.find(bytes, start);
}

auto ByteBlockEditor::findLast(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return ByteBlock{*this}.findLast(bytes);
}

auto ByteBlockEditor::clear() noexcept -> ByteBlockEditor & {
    if (!_data.isNull()) {
        if (_data.isShared()) {
            _data = createData(0U, _data.constGet()->capacity());
        } else {
            _data.get()->setSize(0U);
        }
    }
    return *this;
}

void ByteBlockEditor::reset() noexcept {
    _data.reset();
}

auto ByteBlockEditor::remove(const ByteRange range) -> ByteBlockEditor & {
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
    data->setSize(static_cast<ByteBlockData::SizeType>(oldLength - count));
    return *this;
}

auto ByteBlockEditor::keep(const ByteRange range) -> ByteBlockEditor & {
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
    data->setSize(static_cast<ByteBlockData::SizeType>(count));
    return *this;
}

auto ByteBlockEditor::replace(const ByteRange range, const ByteBlock &replacement) -> ByteBlockEditor & {
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
    data->setSize(static_cast<ByteBlockData::SizeType>(newLength));
    return *this;
}

auto ByteBlockEditor::insert(const ByteIndex index, const ByteBlock &bytes) -> ByteBlockEditor & {
    if (index.isNoIndex()) {
        return *this;
    }
    const auto insertIndex = std::min(index.toSizeT(), length().toSizeT());
    return replace(ByteRange::emptyAt(ByteIndex::fromSizeT(insertIndex)), bytes);
}

auto ByteBlockEditor::append(const Byte value) -> ByteBlockEditor & {
    const auto oldLength = length().toSizeT();
    ensureCapacity(oldLength + 1U);
    auto *data = dataForWrite();
    data->data()[oldLength] = value;
    data->setSize(static_cast<ByteBlockData::SizeType>(oldLength + 1U));
    return *this;
}

auto ByteBlockEditor::append(const ByteBlock &bytes) -> ByteBlockEditor & {
    const auto appendBytes = bytes.toByteVector();
    if (appendBytes.empty()) {
        return *this;
    }
    const auto oldLength = length().toSizeT();
    const auto newLength = oldLength + appendBytes.size();
    ensureCapacity(newLength);
    auto *data = dataForWrite();
    std::memcpy(data->data() + oldLength, appendBytes.data(), appendBytes.size() * sizeof(Byte));
    data->setSize(static_cast<ByteBlockData::SizeType>(newLength));
    return *this;
}

auto ByteBlockEditor::removed(const ByteRange range) const -> ByteBlockEditor {
    auto result = *this;
    result.remove(range);
    return result;
}

auto ByteBlockEditor::replaced(const ByteRange range, const ByteBlock &replacement) const -> ByteBlockEditor {
    auto result = *this;
    result.replace(range, replacement);
    return result;
}

auto ByteBlockEditor::join(const std::vector<ByteBlock> &parts) const -> ByteBlockEditor {
    if (parts.empty()) {
        return {};
    }
    auto result = ByteBlockEditor{};
    for (auto i = std::size_t{0}; i < parts.size(); ++i) {
        if (i > 0U) {
            result.append(ByteBlock{*this});
        }
        result.append(parts[i]);
    }
    return result;
}

auto ByteBlockEditor::toByteVector() const -> std::vector<Byte> {
    return ByteBlock{*this}.toByteVector();
}

auto ByteBlockEditor::toUInt8Vector() const -> std::vector<uint8_t> {
    return ByteBlock{*this}.toUInt8Vector();
}

auto ByteBlockEditor::toCharVector() const -> std::vector<char> {
    return ByteBlock{*this}.toCharVector();
}

void ByteBlockEditor::detach() {
    ensureUnique();
}

auto ByteBlockEditor::capacity() const noexcept -> ByteLength {
    if (_data.isNull()) {
        return ByteLength::zero();
    }
    return ByteLength::fromSizeT(_data.constGet()->capacity());
}

void ByteBlockEditor::reserve(const ByteLength capacity) {
    if (capacity <= this->capacity()) {
        return;
    }
    ensureCapacity(capacity.toSizeTOrThrow());
}

void ByteBlockEditor::shrinkToFit() {
    const auto currentLength = length().toSizeT();
    if (currentLength == 0U) {
        _data.reset();
        return;
    }
    if (!_data.isShared() && capacity().toSizeT() == currentLength) {
        return;
    }
    _data = createData(ByteBlock{*this}.dataSpan());
}

auto ByteBlockEditor::fromJoined(const std::vector<ByteBlock> &parts) -> ByteBlockEditor {
    auto result = ByteBlockEditor{};
    for (const auto &part : parts) {
        result.append(part);
    }
    return result;
}

void ByteBlockEditor::ensureCapacity(const std::size_t requiredCapacity) {
    const auto oldLength = length().toSizeT();
    impl::ensureSharedArrayCapacity(
        _data, oldLength, requiredCapacity, false, [oldLength](const auto *oldData, auto *newData) -> void {
            if (oldData != nullptr && oldLength > 0U) {
                std::memcpy(newData->data(), oldData->data(), oldLength * sizeof(Byte));
            }
        });
}

void ByteBlockEditor::ensureUnique() {
    if (_data.isNull()) {
        return;
    }
    _data.detach();
}

auto ByteBlockEditor::createData(const std::size_t size, const std::size_t capacity) -> ByteBlockDataPtr {
    if (size == 0U && capacity == 0U) {
        return {};
    }
    return ByteBlockDataPtr{ByteBlockData::create(
        static_cast<ByteBlockData::SizeType>(size), static_cast<ByteBlockData::SizeType>(capacity))};
}

auto ByteBlockEditor::createData(const std::span<const Byte> bytes) -> ByteBlockDataPtr {
    auto data = createData(bytes.size(), bytes.size());
    if (!bytes.empty()) {
        std::memcpy(data.get()->data(), bytes.data(), bytes.size() * sizeof(Byte));
    }
    return data;
}

auto ByteBlockEditor::dataForWrite() -> ByteBlockData * {
    return _data.get();
}

}
