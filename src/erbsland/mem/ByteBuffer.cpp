// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBuffer.hpp"

#include "impl/ByteBufferData.hpp"
#include "impl/ByteComparisonTools.hpp"
#include "impl/ByteDataView.hpp"
#include "impl/ByteModifyTools.hpp"
#include "impl/ByteReadTools.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::mem {

using namespace text::literals;
using ModifyTools = impl::ByteModifyTools<impl::ByteBufferData, impl::ByteBufferData>;
using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteRange;

ByteBuffer::ByteBuffer(const ByteLength length, const Byte value) :
    _data{ModifyTools::createData(length.toSizeTOrThrow(), length.toSizeTOrThrow())} {
    fill(value);
}

ByteBuffer::ByteBuffer(const std::initializer_list<Byte> bytes) :
    _data{ModifyTools::createData(ConstByteSpan{bytes.begin(), bytes.size()})} {
}

ByteBuffer::ByteBuffer(const ConstByteSpan bytes) : _data{ModifyTools::createData(bytes)} {
}

ByteBuffer::~ByteBuffer() = default;

ByteBuffer::ByteBuffer(const ByteBuffer &other) : _data{ModifyTools::createData(other.span(), other.isSensitive())} {
}

ByteBuffer::ByteBuffer(ByteBuffer &&other) noexcept : _data{std::move(other._data)} {
}

auto ByteBuffer::operator=(const ByteBuffer &other) -> ByteBuffer & {
    if (this == &other) {
        return *this;
    }
    auto replacement = ByteBuffer{other};
    swap(replacement);
    return *this;
}

auto ByteBuffer::operator=(ByteBuffer &&other) noexcept -> ByteBuffer & {
    if (this == &other) {
        return *this;
    }
    auto replacement = ByteBuffer{std::move(other)};
    swap(replacement);
    return *this;
}

auto ByteBuffer::operator<=>(const ByteBuffer &other) const noexcept -> std::strong_ordering {
    return impl::ByteComparisonTools{dataView()}.compare(other.dataView());
}

auto ByteBuffer::isEqualConstTime(const ByteBuffer &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(other.dataView());
}

auto ByteBuffer::isEqualConstTime(const ConstByteSpan other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(impl::ByteDataView{other});
}

auto ByteBuffer::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto ByteBuffer::length() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_data.size());
}

auto ByteBuffer::capacity() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_data.capacity());
}

auto ByteBuffer::span() const noexcept -> ConstByteSpan {
    return dataView().dataSpan();
}

auto ByteBuffer::get(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return impl::ByteReadTools{dataView()}.get(index, defaultValue);
}

auto ByteBuffer::getOrThrow(const ByteIndex index) const -> Byte {
    return impl::ByteReadTools{dataView()}.getOrThrow(index);
}

void ByteBuffer::setSensitive(const bool sensitive) noexcept {
    if (isSensitive() == sensitive) {
        return;
    }
    if (sensitive) {
        _data.setSensitive();
        return;
    }
    ModifyTools{_data}.secureErase();
    _data.setSize(0U);
    _data.setFlags(_data.flags() & static_cast<std::uint8_t>(~impl::ByteBufferData::cSensitiveFlag));
}

void ByteBuffer::set(const ByteIndex index, const Byte value) noexcept {
    impl::ByteWriteTools{writableSpan()}.set(index, value);
}

void ByteBuffer::setOrThrow(const ByteIndex index, const Byte value) {
    impl::ByteWriteTools{writableSpan()}.setOrThrow(index, value);
}

void ByteBuffer::xorAt(const ByteIndex index, const Byte value) noexcept {
    impl::ByteWriteTools{writableSpan()}.xorAt(index, value);
}

void ByteBuffer::xorAtOrThrow(const ByteIndex index, const Byte value) {
    impl::ByteWriteTools{writableSpan()}.xorAtOrThrow(index, value);
}

void ByteBuffer::overwrite(const ConstByteSpan source) {
    overwrite(ByteRange::all(), source);
}

void ByteBuffer::overwrite(const ByteIndex index, const ConstByteSpan source) {
    overwrite(ByteRange{index, ByteLength::infinite()}, source);
}

void ByteBuffer::overwrite(const ByteRange targetRange, const ConstByteSpan source) {
    ModifyTools{_data}.overwrite(targetRange, impl::ByteDataView{source}, false);
}

auto ByteBuffer::xorWith(const ConstByteSpan source) -> bool {
    return ModifyTools{_data}.xorWith(impl::ByteDataView{source}, false);
}

void ByteBuffer::xorWithOrThrow(const ConstByteSpan source) {
    if (!xorWith(source)) {
        throw err::ParameterError{"XOR operands must have equal lengths."_el, "source"_el};
    }
}

void ByteBuffer::xorWith(const ByteRange targetRange, const ConstByteSpan source) {
    ModifyTools{_data}.xorWith(targetRange, impl::ByteDataView{source}, false);
}

void ByteBuffer::secureErase() noexcept {
    ModifyTools{_data}.secureErase();
}

auto ByteBuffer::resize(const ByteLength lengthValue) -> ByteBuffer & {
    ModifyTools{_data}.resize(lengthValue);
    return *this;
}

void ByteBuffer::reserve(const ByteLength capacityValue) {
    ModifyTools{_data}.reserve(capacityValue);
}

void ByteBuffer::shrinkToFit() {
    ModifyTools{_data}.shrinkToFit();
}

auto ByteBuffer::clear() noexcept -> ByteBuffer & {
    ModifyTools{_data}.clear();
    return *this;
}

void ByteBuffer::reset() noexcept {
    _data.reset();
}

auto ByteBuffer::append(const Byte value, const unit::ByteLength length) -> ByteBuffer & {
    ModifyTools{_data}.append(value, length);
    return *this;
}

auto ByteBuffer::append(const ConstByteSpan bytes) -> ByteBuffer & {
    ModifyTools{_data}.append(impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBuffer::insert(const ByteIndex index, const ConstByteSpan bytes) -> ByteBuffer & {
    ModifyTools{_data}.insert(index, impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBuffer::replace(const ByteRange range, const ConstByteSpan replacement) -> ByteBuffer & {
    ModifyTools{_data}.replace(range, impl::ByteDataView{replacement}, false);
    return *this;
}

auto ByteBuffer::remove(const ByteRange range) -> ByteBuffer & {
    ModifyTools{_data}.remove(range);
    return *this;
}

auto ByteBuffer::keep(const ByteRange range) -> ByteBuffer & {
    ModifyTools{_data}.keep(range);
    return *this;
}

auto ByteBuffer::toUInt8Vector() const -> std::vector<uint8_t> {
    return impl::ByteReadTools{dataView()}.toUInt8Vector();
}

auto ByteBuffer::toCharVector() const -> std::vector<char> {
    return impl::ByteReadTools{dataView()}.toCharVector();
}

auto ByteBuffer::fromSpan(const std::span<const std::byte> bytes) -> ByteBuffer {
    return ByteBuffer{toConstByteSpan(bytes)};
}

auto ByteBuffer::fromSpan(const std::span<const uint8_t> bytes) -> ByteBuffer {
    return ByteBuffer{toConstByteSpan(bytes)};
}

auto ByteBuffer::fromSpan(const std::span<const char> bytes) -> ByteBuffer {
    return ByteBuffer{toConstByteSpan(bytes)};
}

auto ByteBuffer::dataView() const noexcept -> impl::ByteDataView {
    return impl::ByteDataView{ConstByteSpan{_data.data(), _data.size()}};
}

auto ByteBuffer::writableSpan() noexcept -> ByteSpan {
    return ByteSpan{_data.data(), _data.size()};
}

void ByteBuffer::swap(ByteBuffer &other) noexcept {
    _data.swap(other._data);
}

}
