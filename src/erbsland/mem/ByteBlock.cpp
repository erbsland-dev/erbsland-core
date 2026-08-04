// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlock.hpp"

#include "ByteBlockEditor.hpp"

#include "impl/ByteBlockData.hpp"
#include "impl/ByteComparisonTools.hpp"
#include "impl/ByteReadTools.hpp"
#include "impl/SecureErase.hpp"

#include "../util/HashHelper.hpp"

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

ByteBlock::ByteBlock(const std::initializer_list<Byte> bytes) :
    ByteBlock{ByteBlockEditor::fromSpan(ConstByteSpan{bytes.begin(), bytes.size()})} {
}

ByteBlock::ByteBlock(const ByteBlockEditor &editor) noexcept :
    _data{editor._data}, _range{ByteRange::fromLength(editor.length())} {
}

ByteBlock::ByteBlock(impl::ByteBlockDataPtr data, ByteRange range) noexcept : _data{std::move(data)}, _range{range} {
}

auto ByteBlock::isSensitive() const noexcept -> bool {
    return !_data.isNull() && _data.constGet()->isSensitive();
}

void ByteBlock::markAsSensitive() noexcept {
    if (!_data.isNull()) {
        _data.constGet()->setSensitive();
    }
}

auto ByteBlock::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    return impl::ByteComparisonTools{dataView()}.compare(other.dataView());
}

auto ByteBlock::operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering {
    return impl::ByteComparisonTools{dataView()}.compare(other.dataView());
}

auto ByteBlock::isEqualConstTime(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(other.dataView());
}

auto ByteBlock::isEqualConstTime(const ConstByteSpan other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(impl::ByteDataView{other});
}

auto ByteBlock::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto ByteBlock::startsWith(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.startsWith(other.dataView());
}

auto ByteBlock::startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    return startsWith(ConstByteSpan{byteSequence.begin(), byteSequence.size()});
}

auto ByteBlock::startsWith(const ConstByteSpan byteSequence) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.startsWith(impl::ByteDataView{byteSequence});
}

auto ByteBlock::endsWith(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.endsWith(other.dataView());
}

auto ByteBlock::endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    return endsWith(ConstByteSpan{byteSequence.begin(), byteSequence.size()});
}

auto ByteBlock::endsWith(const ConstByteSpan byteSequence) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.endsWith(impl::ByteDataView{byteSequence});
}

auto ByteBlock::contains(const ByteBlock &other) const noexcept -> bool {
    return !find(other).isNoIndex();
}

auto ByteBlock::contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool {
    return contains(ConstByteSpan{byteSequence.begin(), byteSequence.size()});
}

auto ByteBlock::contains(const ConstByteSpan byteSequence) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.contains(impl::ByteDataView{byteSequence});
}

auto ByteBlock::length() const noexcept -> ByteLength {
    return dataView().length();
}

auto ByteBlock::get(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return impl::ByteReadTools{dataView()}.get(index, defaultValue);
}

auto ByteBlock::getOrThrow(const ByteIndex index) const -> Byte {
    return impl::ByteReadTools{dataView()}.getOrThrow(index);
}

auto ByteBlock::slice(const ByteRange range) const noexcept -> ByteBlock {
    const auto absoluteRange = impl::ByteReadTools{dataView()}.sliceRange(range);
    if (_data.isNull() || absoluteRange.isEmpty()) {
        return {};
    }
    return ByteBlock{_data, absoluteRange};
}

auto ByteBlock::slice(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, end});
}

auto ByteBlock::slice(const ByteIndex begin, const ByteLength length) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, length});
}

void ByteBlock::secureErase() {
    if (isEmpty()) {
        return;
    }
    if (_data.isShared()) {
        auto replacement = ByteBlockEditor{length()};
        if (isSensitive()) {
            replacement.markAsSensitive();
        }
        *this = ByteBlock{replacement};
        return;
    }
    auto *data = _data.get();
    impl::secureErase(std::as_writable_bytes(std::span{data->data(), data->capacity()}));
}

auto ByteBlock::find(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(bytes.dataView());
}

auto ByteBlock::find(std::initializer_list<Byte> byteSequence) const noexcept -> ByteIndex {
    return find(ConstByteSpan{byteSequence.begin(), byteSequence.size()});
}

auto ByteBlock::find(const ConstByteSpan byteSequence) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(impl::ByteDataView{byteSequence});
}

auto ByteBlock::find(const ByteBlock &bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(bytes.dataView(), start);
}

auto ByteBlock::find(std::initializer_list<Byte> byteSequence, ByteIndex start) const noexcept -> ByteIndex {
    return find(ConstByteSpan{byteSequence.begin(), byteSequence.size()}, start);
}

auto ByteBlock::find(const ConstByteSpan byteSequence, const ByteIndex start) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(impl::ByteDataView{byteSequence}, start);
}

auto ByteBlock::findLast(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.findLast(bytes.dataView());
}

auto ByteBlock::findLast(std::initializer_list<Byte> byteSequence) const noexcept -> ByteIndex {
    return findLast(ConstByteSpan{byteSequence.begin(), byteSequence.size()});
}

auto ByteBlock::findLast(const ConstByteSpan byteSequence) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.findLast(impl::ByteDataView{byteSequence});
}

auto ByteBlock::toByteBuffer() const -> ByteBuffer {
    return ByteBuffer{span()};
}

auto ByteBlock::fromSpan(const ConstByteSpan bytes) -> ByteBlock {
    return ByteBlock{ByteBlockEditor::fromSpan(bytes)};
}

auto ByteBlock::fromSpan(const std::span<const std::byte> bytes) -> ByteBlock {
    return ByteBlock{ByteBlockEditor::fromSpan(bytes)};
}

auto ByteBlock::fromSpan(const std::span<const uint8_t> bytes) -> ByteBlock {
    return ByteBlock{ByteBlockEditor::fromSpan(bytes)};
}

auto ByteBlock::fromSpan(const std::span<const char> bytes) -> ByteBlock {
    return ByteBlock{ByteBlockEditor::fromSpan(bytes)};
}

auto ByteBlock::fromVector(const std::vector<uint8_t> &bytes) -> ByteBlock {
    return fromSpan(std::span<const uint8_t>{bytes});
}

auto ByteBlock::fromVector(const std::vector<char> &bytes) -> ByteBlock {
    return fromSpan(std::span<const char>{bytes});
}

auto ByteBlock::toUInt8Vector() const -> std::vector<uint8_t> {
    return impl::ByteReadTools{dataView()}.toUInt8Vector();
}

auto ByteBlock::toCharVector() const -> std::vector<char> {
    return impl::ByteReadTools{dataView()}.toCharVector();
}

auto ByteBlock::dataView() const noexcept -> impl::ByteDataView {
    if (_data.isNull()) {
        return {};
    }
    const auto *data = _data.constGet();
    return impl::ByteDataView{ConstByteSpan{data->data(), static_cast<std::size_t>(data->size())}, _range};
}

auto ByteBlock::storageId() const noexcept -> std::size_t {
    return util::createHash(_data.storageId(), _range);
}

}
