// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlock.hpp"

#include "ByteBlockEditor.hpp"

#include "impl/ByteBlockData.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../util/HashHelper.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::mem {

using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteRange;

ByteBlock::ByteBlock() = default;

ByteBlock::~ByteBlock() = default;

ByteBlock::ByteBlock(const ByteBlock &) = default;

ByteBlock::ByteBlock(ByteBlock &&) noexcept = default;

auto ByteBlock::operator=(const ByteBlock &) -> ByteBlock & = default;

auto ByteBlock::operator=(ByteBlock &&) noexcept -> ByteBlock & = default;

ByteBlock::ByteBlock(const ByteLength length, const Byte value) : ByteBlock{ByteBlockEditor{length, value}} {
}

ByteBlock::ByteBlock(const std::span<const Byte> bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const std::span<const uint8_t> bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const std::span<const char> bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const std::vector<Byte> &bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const std::vector<uint8_t> &bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const std::vector<char> &bytes) : ByteBlock{ByteBlockEditor{bytes}} {
}

ByteBlock::ByteBlock(const ByteBlockEditor &editor) noexcept :
    _data{editor._data}, _range{ByteRange::fromLength(editor.length())} {
}

ByteBlock::ByteBlock(impl::ByteBlockDataPtr data, ByteRange range) noexcept : _data{std::move(data)}, _range{range} {
}

auto ByteBlock::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    const auto left = dataSpan();
    const auto right = other.dataSpan();
    const auto size = std::min(left.size(), right.size());
    for (auto i = std::size_t{0}; i < size; ++i) {
        if (const auto result = left[i] <=> right[i]; result != std::strong_ordering::equal) {
            return result;
        }
    }
    return left.size() <=> right.size();
}

auto ByteBlock::operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering {
    return *this <=> ByteBlock{other};
}

auto ByteBlock::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto ByteBlock::startsWith(const ByteBlock &other) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = other.dataSpan();
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlock::startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlock::startsWith(const std::span<const Byte> byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    return data.size() >= byteSequence.size() &&
        std::ranges::equal(byteSequence, data.subspan(0U, byteSequence.size()));
}

auto ByteBlock::startsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlock::endsWith(const ByteBlock &other) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = other.dataSpan();
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlock::endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlock::endsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlock::contains(const ByteBlock &other) const noexcept -> bool {
    return !find(other).isNoIndex();
}

auto ByteBlock::contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    return !find(byteSequence).isNoIndex();
}

auto ByteBlock::contains(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    return !find(byteSequence).isNoIndex();
}

auto ByteBlock::length() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(dataSpan().size());
}

auto ByteBlock::get(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    const auto data = dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        return defaultValue;
    }
    return data[index.toSizeT()];
}

auto ByteBlock::getOrThrow(const ByteIndex index) const -> Byte {
    const auto data = dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        throw err::OutOfRangeError{"Read position out of range"};
    }
    return data[index.toSizeT()];
}

auto ByteBlock::slice(const ByteRange range) const noexcept -> ByteBlock {
    if (_data.isNull() || !_range.isValid()) {
        return {};
    }
    const auto clampedRange = range.clampedTo(length());
    if (clampedRange.isEmpty()) {
        return {};
    }
    return ByteBlock{_data, clampedRange.withOrigin(_range.index())};
}

auto ByteBlock::slice(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, end});
}

auto ByteBlock::slice(const ByteIndex begin, const ByteLength length) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, length});
}

auto ByteBlock::find(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return find(bytes, ByteIndex::zero());
}

auto ByteBlock::find(std::initializer_list<Byte> byteSequence) const noexcept -> ByteIndex {
    return findImpl(byteSequence, ByteIndex::zero());
}

auto ByteBlock::find(const std::vector<Byte> &byteSequence) const noexcept -> ByteIndex {
    return findImpl(byteSequence, ByteIndex::zero());
}

auto ByteBlock::find(const ByteBlock &bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return findImpl(bytes.dataSpan(), start);
}

auto ByteBlock::findImpl(const std::span<const Byte> needle, ByteIndex start) const noexcept -> ByteIndex {
    if (start.isNoIndex()) {
        return ByteIndex::noIndex();
    }

    const auto data = dataSpan();
    if (start.toSizeT() > data.size()) {
        return ByteIndex::noIndex();
    }

    if (needle.empty()) {
        return start;
    }
    if (needle.size() > data.size() - start.toSizeT()) {
        return ByteIndex::noIndex();
    }

    const auto searchRange = data.subspan(start.toSizeT());
    const auto result = std::search(searchRange.begin(), searchRange.end(), needle.begin(), needle.end());
    if (result == searchRange.end()) {
        return ByteIndex::noIndex();
    }
    return ByteIndex::fromSizeT(start.toSizeT() + static_cast<std::size_t>(result - searchRange.begin()));
}

auto ByteBlock::find(std::initializer_list<Byte> byteSequence, ByteIndex start) const noexcept -> ByteIndex {
    return findImpl(byteSequence, start);
}

auto ByteBlock::find(const std::vector<Byte> &byteSequence, ByteIndex start) const noexcept -> ByteIndex {
    return findImpl(byteSequence, start);
}

auto ByteBlock::findLast(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return findLastImpl(bytes.dataSpan());
}

auto ByteBlock::findLastImpl(const std::span<const Byte> needle) const noexcept -> ByteIndex {
    const auto data = dataSpan();
    if (needle.empty()) {
        return ByteIndex::end(length());
    }
    if (needle.size() > data.size()) {
        return ByteIndex::noIndex();
    }

    const auto result = std::find_end(data.begin(), data.end(), needle.begin(), needle.end());
    if (result == data.end()) {
        return ByteIndex::noIndex();
    }
    return ByteIndex::fromSizeT(static_cast<std::size_t>(result - data.begin()));
}

auto ByteBlock::findLast(std::initializer_list<Byte> byteSequence) const noexcept -> ByteIndex {
    return findLastImpl(byteSequence);
}

auto ByteBlock::findLast(const std::vector<Byte> &byteSequence) const noexcept -> ByteIndex {
    return findLastImpl(byteSequence);
}

auto ByteBlock::toByteVector() const -> std::vector<Byte> {
    const auto data = dataSpan();
    return std::vector<Byte>{data.begin(), data.end()};
}

auto ByteBlock::toUInt8Vector() const -> std::vector<uint8_t> {
    const auto data = dataSpan();
    auto result = std::vector<uint8_t>{};
    result.reserve(data.size());
    for (const auto byte : data) {
        result.emplace_back(byte.toRawValue());
    }
    return result;
}

auto ByteBlock::toCharVector() const -> std::vector<char> {
    const auto data = dataSpan();
    auto result = std::vector<char>{};
    result.reserve(data.size());
    for (const auto byte : data) {
        result.emplace_back(static_cast<char>(byte.toRawValue()));
    }
    return result;
}

auto ByteBlock::dataSpan() const noexcept -> std::span<const Byte> {
    if (_data.isNull() || _range.isEmpty() || !_range.isValid()) {
        return {};
    }
    const auto &data = *_data.constGet();
    const auto start = _range.index().toSizeT();
    if (start >= data.size()) {
        return {};
    }
    const auto availableLength = static_cast<std::size_t>(data.size()) - start;
    const auto length = std::min(_range.length().toSizeT(), availableLength);
    return std::span<const Byte>{data.data() + start, length};
}

auto ByteBlock::storageId() const noexcept -> std::size_t {
    return util::createHash(_data.storageId(), _range);
}

}
