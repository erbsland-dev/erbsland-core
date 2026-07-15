// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockView.hpp"

#include "ByteBlock.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../util/HashHelper.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::mem {

ByteBlockView::ByteBlockView(const ByteBlock &block) noexcept :
    _data{block._data}, _range{unit::ByteRange::fromLength(block.length())} {
}

ByteBlockView::ByteBlockView(impl::ByteBlockDataPtr data, unit::ByteRange range) noexcept :
    _data{std::move(data)}, _range{range} {
}

auto ByteBlockView::operator<=>(const ByteBlockView &other) const noexcept -> std::strong_ordering {
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

auto ByteBlockView::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    return *this <=> ByteBlockView{other};
}

auto ByteBlockView::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto ByteBlockView::startsWith(const ByteBlockView &other) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = other.dataSpan();
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlockView::startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlockView::startsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto prefix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= prefix.size() && std::ranges::equal(prefix, data.subspan(0U, prefix.size()));
}

auto ByteBlockView::endsWith(const ByteBlockView &other) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = other.dataSpan();
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlockView::endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlockView::endsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    const auto data = dataSpan();
    const auto suffix = std::span{byteSequence.begin(), byteSequence.size()};
    return data.size() >= suffix.size() && std::ranges::equal(suffix, data.subspan(data.size() - suffix.size()));
}

auto ByteBlockView::contains(const ByteBlockView &other) const noexcept -> bool {
    return !find(other).isNoIndex();
}

auto ByteBlockView::contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    return !find(byteSequence).isNoIndex();
}

auto ByteBlockView::contains(const std::vector<Byte> &byteSequence) const noexcept -> bool {
    return !find(byteSequence).isNoIndex();
}

auto ByteBlockView::length() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(dataSpan().size());
}

auto ByteBlockView::get(const unit::ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    const auto data = dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        return defaultValue;
    }
    return data[index.toSizeT()];
}

auto ByteBlockView::getOrThrow(const unit::ByteIndex index) const -> Byte {
    const auto data = dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        throw err::OutOfRangeError{"Read position out of range"};
    }
    return data[index.toSizeT()];
}

auto ByteBlockView::slice(const unit::ByteRange range) const noexcept -> ByteBlockView {
    if (_data.isNull() || !_range.isValid()) {
        return {};
    }
    const auto clampedRange = range.clampedTo(length());
    if (clampedRange.isEmpty()) {
        return {};
    }
    return ByteBlockView{_data, clampedRange.withOrigin(_range.index())};
}

auto ByteBlockView::slice(const unit::ByteIndex begin, const unit::ByteIndex end) const noexcept -> ByteBlockView {
    return slice(unit::ByteRange{begin, end});
}

auto ByteBlockView::slice(const unit::ByteIndex begin, const unit::ByteLength length) const noexcept -> ByteBlockView {
    return slice(unit::ByteRange{begin, length});
}

auto ByteBlockView::find(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex {
    return find(bytes, unit::ByteIndex::zero());
}

auto ByteBlockView::find(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex {
    return findImpl(byteSequence, unit::ByteIndex::zero());
}

auto ByteBlockView::find(const std::vector<Byte> &byteSequence) const noexcept -> unit::ByteIndex {
    return findImpl(byteSequence, unit::ByteIndex::zero());
}

auto ByteBlockView::find(const ByteBlockView &bytes, const unit::ByteIndex start) const noexcept -> unit::ByteIndex {
    return findImpl(bytes.dataSpan(), start);
}

auto ByteBlockView::findImpl(const std::span<const Byte> needle, unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    if (start.isNoIndex()) {
        return unit::ByteIndex::noIndex();
    }

    const auto data = dataSpan();
    if (start.toSizeT() > data.size()) {
        return unit::ByteIndex::noIndex();
    }

    if (needle.empty()) {
        return start;
    }
    if (needle.size() > data.size() - start.toSizeT()) {
        return unit::ByteIndex::noIndex();
    }

    const auto searchRange = data.subspan(start.toSizeT());
    const auto result = std::search(searchRange.begin(), searchRange.end(), needle.begin(), needle.end());
    if (result == searchRange.end()) {
        return unit::ByteIndex::noIndex();
    }
    return unit::ByteIndex::fromSizeT(start.toSizeT() + static_cast<std::size_t>(result - searchRange.begin()));
}

auto ByteBlockView::find(std::initializer_list<Byte> byteSequence, unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    return findImpl(byteSequence, start);
}

auto ByteBlockView::find(const std::vector<Byte> &byteSequence, unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    return findImpl(byteSequence, start);
}

auto ByteBlockView::findLast(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex {
    return findLastImpl(bytes.dataSpan());
}

auto ByteBlockView::findLastImpl(const std::span<const Byte> needle) const noexcept -> unit::ByteIndex {
    const auto data = dataSpan();
    if (needle.empty()) {
        return unit::ByteIndex::end(length());
    }
    if (needle.size() > data.size()) {
        return unit::ByteIndex::noIndex();
    }

    const auto result = std::find_end(data.begin(), data.end(), needle.begin(), needle.end());
    if (result == data.end()) {
        return unit::ByteIndex::noIndex();
    }
    return unit::ByteIndex::fromSizeT(static_cast<std::size_t>(result - data.begin()));
}

auto ByteBlockView::findLast(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex {
    return findLastImpl(byteSequence);
}

auto ByteBlockView::findLast(const std::vector<Byte> &byteSequence) const noexcept -> unit::ByteIndex {
    return findLastImpl(byteSequence);
}

auto ByteBlockView::toByteVector() const -> std::vector<Byte> {
    const auto data = dataSpan();
    return std::vector<Byte>{data.begin(), data.end()};
}

auto ByteBlockView::toUInt8Vector() const -> std::vector<uint8_t> {
    const auto data = dataSpan();
    auto result = std::vector<uint8_t>{};
    result.reserve(data.size());
    for (const auto byte : data) {
        result.emplace_back(byte.toRawValue());
    }
    return result;
}

auto ByteBlockView::toCharVector() const -> std::vector<char> {
    const auto data = dataSpan();
    auto result = std::vector<char>{};
    result.reserve(data.size());
    for (const auto byte : data) {
        result.emplace_back(static_cast<char>(byte.toRawValue()));
    }
    return result;
}

auto ByteBlockView::dataSpan() const noexcept -> std::span<const Byte> {
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

auto ByteBlockView::storageId() const noexcept -> std::size_t {
    return util::createHash(_data.storageId(), _range);
}

}
