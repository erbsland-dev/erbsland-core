// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlock.hpp"

#include "ByteBlockEditor.hpp"
#include "ByteBlockLiteral.hpp"

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
    _storage{editor._data}, _range{ByteRange::fromLength(editor.length())} {
}

ByteBlock::ByteBlock(ByteBlockLiteral literal) noexcept :
    _storage{literal}, _range{ByteRange::fromLength(literal.length())} {
}

ByteBlock::ByteBlock(impl::ByteBlockDataPtr data, ByteRange range) noexcept : _storage{std::move(data)}, _range{range} {
}

ByteBlock::ByteBlock(impl::ByteBlockStorage storage, ByteRange range) noexcept :
    _storage{std::move(storage)}, _range{range} {
}

auto ByteBlock::copy() const -> ByteBlock {
    auto result = ByteBlockEditor::fromSpan(span());
    if (isSensitive()) {
        result.markAsSensitive();
    }
    return ByteBlock{result};
}

auto ByteBlock::isSensitive() const noexcept -> bool {
    const auto *data = std::get_if<impl::ByteBlockDataPtr>(&_storage);
    return data != nullptr && !data->isNull() && data->constGet()->isSensitive();
}

void ByteBlock::markAsSensitive() noexcept {
    if (isEmpty()) {
        return;
    }
    if (auto *data = std::get_if<impl::ByteBlockDataPtr>(&_storage)) {
        data->constGet()->setSensitive();
        return;
    }
    if (std::holds_alternative<ByteBlockLiteral>(_storage)) {
        auto replacement = ByteBlockEditor::fromSpan(span());
        replacement.markAsSensitive();
        *this = ByteBlock{replacement};
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
    if (std::holds_alternative<std::monostate>(_storage) || absoluteRange.isEmpty()) {
        return {};
    }
    return ByteBlock{_storage, absoluteRange};
}

auto ByteBlock::slice(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, end});
}

auto ByteBlock::slice(const ByteIndex begin, const ByteLength length) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, length});
}

auto ByteBlock::kept(const ByteRange range) const -> ByteBlock {
    auto result = ByteBlockEditor::fromSpan(span(range));
    if (isSensitive()) {
        result.markAsSensitive();
    }
    return ByteBlock{result};
}

void ByteBlock::secureErase() {
    if (isEmpty()) {
        return;
    }
    auto *data = std::get_if<impl::ByteBlockDataPtr>(&_storage);
    if (data == nullptr || data->isShared()) {
        auto replacement = ByteBlockEditor{length()};
        if (isSensitive()) {
            replacement.markAsSensitive();
        }
        *this = ByteBlock{replacement};
        return;
    }
    auto *sharedData = data->get();
    impl::secureErase(std::as_writable_bytes(std::span{sharedData->data(), sharedData->capacity()}));
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
    if (const auto *data = std::get_if<impl::ByteBlockDataPtr>(&_storage)) {
        if (data->isNull()) {
            return {};
        }
        const auto *sharedData = data->constGet();
        return impl::ByteDataView{
            ConstByteSpan{sharedData->data(), static_cast<std::size_t>(sharedData->size())}, _range};
    }
    if (const auto *literal = std::get_if<ByteBlockLiteral>(&_storage)) {
        return impl::ByteDataView{literal->span(), _range};
    }
    return {};
}

auto ByteBlock::storageId() const noexcept -> std::size_t {
    if (const auto *data = std::get_if<impl::ByteBlockDataPtr>(&_storage)) {
        return util::createHash(data->storageId(), _range);
    }
    if (const auto *literal = std::get_if<ByteBlockLiteral>(&_storage)) {
        return util::createHash(static_cast<const void *>(literal->span().data()), _range);
    }
    return {};
}

}
