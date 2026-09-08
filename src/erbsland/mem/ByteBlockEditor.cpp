// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockEditor.hpp"

#include "ByteBlock.hpp"

#include "impl/ByteComparisonTools.hpp"
#include "impl/ByteModifyTools.hpp"
#include "impl/ByteReadTools.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::mem {

using namespace text::literals;
using impl::ByteBlockDataPtr;
using impl::ByteModifyTools;
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
    _data{ByteModifyTools<>::createData(length.toSizeTOrThrow(), length.toSizeTOrThrow())} {
    if (!_data.isNull()) {
        impl::ByteWriteTools{ByteSpan{_data.get()->data(), static_cast<std::size_t>(_data.get()->size())}}.fill(
            ByteRange::all(), value);
    }
}

ByteBlockEditor::ByteBlockEditor(const std::initializer_list<Byte> bytes) :
    _data{ByteModifyTools<>::createData(ConstByteSpan{bytes.begin(), bytes.size()})} {
}

ByteBlockEditor::ByteBlockEditor(const ByteBlock &block) :
    _data{ByteModifyTools<>::createData(block.dataView().dataSpan(), block.isSensitive())} {
}

ByteBlockEditor::ByteBlockEditor(ByteBlockDataPtr data) noexcept : _data{std::move(data)} {
}

auto ByteBlockEditor::copy() const -> ByteBlockEditor {
    return ByteBlockEditor{ByteModifyTools<>::createData(span(), isSensitive())};
}

auto ByteBlockEditor::isSensitive() const noexcept -> bool {
    return !_data.isNull() && _data.constGet()->isSensitive();
}

void ByteBlockEditor::markAsSensitive() noexcept {
    if (!_data.isNull()) {
        _data.constGet()->setSensitive();
    }
}

auto ByteBlockEditor::operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering {
    return impl::ByteComparisonTools{dataView()}.compare(other.dataView());
}

auto ByteBlockEditor::operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering {
    return impl::ByteComparisonTools{dataView()}.compare(other.dataView());
}

auto ByteBlockEditor::isEqualConstTime(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(other.dataView());
}

auto ByteBlockEditor::isEqualConstTime(const ConstByteSpan other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.isEqualConstTime(impl::ByteDataView{other});
}

auto ByteBlockEditor::startsWith(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.startsWith(other.dataView());
}

auto ByteBlockEditor::startsWith(const std::initializer_list<Byte> bytes) const noexcept -> bool {
    return startsWith(ConstByteSpan{bytes.begin(), bytes.size()});
}

auto ByteBlockEditor::startsWith(const ConstByteSpan bytes) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.startsWith(impl::ByteDataView{bytes});
}

auto ByteBlockEditor::endsWith(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.endsWith(other.dataView());
}

auto ByteBlockEditor::endsWith(const std::initializer_list<Byte> bytes) const noexcept -> bool {
    return endsWith(ConstByteSpan{bytes.begin(), bytes.size()});
}

auto ByteBlockEditor::endsWith(const ConstByteSpan bytes) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.endsWith(impl::ByteDataView{bytes});
}

auto ByteBlockEditor::contains(const ByteBlock &other) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.contains(other.dataView());
}

auto ByteBlockEditor::contains(const std::initializer_list<Byte> bytes) const noexcept -> bool {
    return contains(ConstByteSpan{bytes.begin(), bytes.size()});
}

auto ByteBlockEditor::contains(const ConstByteSpan bytes) const noexcept -> bool {
    return impl::ByteComparisonTools{dataView()}.contains(impl::ByteDataView{bytes});
}

auto ByteBlockEditor::length() const noexcept -> ByteLength {
    return dataView().length();
}

auto ByteBlockEditor::get(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return impl::ByteReadTools{dataView()}.get(index, defaultValue);
}

auto ByteBlockEditor::getOrThrow(const ByteIndex index) const -> Byte {
    return impl::ByteReadTools{dataView()}.getOrThrow(index);
}

void ByteBlockEditor::set(const ByteIndex index, const Byte value) {
    if (!index.isValid() || index.toSizeT() >= length().toSizeT()) {
        return;
    }
    impl::ByteWriteTools{writableSpan()}.set(index, value);
}

void ByteBlockEditor::setOrThrow(const ByteIndex index, const Byte value) {
    if (!index.isValid() || index.toSizeT() >= length().toSizeT()) {
        throw err::OutOfRangeError{"Write position out of range"};
    }
    impl::ByteWriteTools{writableSpan()}.setOrThrow(index, value);
}

void ByteBlockEditor::xorAt(const ByteIndex index, const Byte value) {
    if (!index.isValid() || index.toSizeT() >= length().toSizeT()) {
        return;
    }
    impl::ByteWriteTools{writableSpan()}.xorAt(index, value);
}

void ByteBlockEditor::xorAtOrThrow(const ByteIndex index, const Byte value) {
    if (!index.isValid() || index.toSizeT() >= length().toSizeT()) {
        throw err::OutOfRangeError{"Write position out of range"};
    }
    impl::ByteWriteTools{writableSpan()}.xorAtOrThrow(index, value);
}

auto ByteBlockEditor::slice(const ByteRange range) const noexcept -> ByteBlock {
    const auto absoluteRange = impl::ByteReadTools{dataView()}.sliceRange(range);
    if (_data.isNull() || absoluteRange.isEmpty()) {
        return {};
    }
    return ByteBlock{_data, absoluteRange};
}

auto ByteBlockEditor::slice(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, end});
}

auto ByteBlockEditor::slice(const ByteIndex begin, const ByteLength length) const noexcept -> ByteBlock {
    return slice(ByteRange{begin, length});
}

auto ByteBlockEditor::find(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(bytes.dataView());
}

auto ByteBlockEditor::find(const std::initializer_list<Byte> bytes) const noexcept -> ByteIndex {
    return find(ConstByteSpan{bytes.begin(), bytes.size()});
}

auto ByteBlockEditor::find(const ConstByteSpan bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(impl::ByteDataView{bytes});
}

auto ByteBlockEditor::find(const ByteBlock &bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(bytes.dataView(), start);
}

auto ByteBlockEditor::find(const std::initializer_list<Byte> bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return find(ConstByteSpan{bytes.begin(), bytes.size()}, start);
}

auto ByteBlockEditor::find(const ConstByteSpan bytes, const ByteIndex start) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.find(impl::ByteDataView{bytes}, start);
}

auto ByteBlockEditor::findLast(const ByteBlock &bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.findLast(bytes.dataView());
}

auto ByteBlockEditor::findLast(const std::initializer_list<Byte> bytes) const noexcept -> ByteIndex {
    return findLast(ConstByteSpan{bytes.begin(), bytes.size()});
}

auto ByteBlockEditor::findLast(const ConstByteSpan bytes) const noexcept -> ByteIndex {
    return impl::ByteComparisonTools{dataView()}.findLast(impl::ByteDataView{bytes});
}

auto ByteBlockEditor::clear() noexcept -> ByteBlockEditor & {
    ByteModifyTools{_data}.clear();
    return *this;
}

void ByteBlockEditor::reset() noexcept {
    ByteModifyTools{_data}.reset();
}

void ByteBlockEditor::secureErase() {
    ByteModifyTools{_data}.secureErase();
}

auto ByteBlockEditor::resize(const ByteLength length) -> ByteBlockEditor & {
    ByteModifyTools{_data}.resize(length);
    return *this;
}

auto ByteBlockEditor::remove(const ByteRange range) -> ByteBlockEditor & {
    ByteModifyTools{_data}.remove(range);
    return *this;
}

auto ByteBlockEditor::keep(const ByteRange range) -> ByteBlockEditor & {
    ByteModifyTools{_data}.keep(range);
    return *this;
}

auto ByteBlockEditor::replace(const ByteRange range, const ByteBlock &replacement) -> ByteBlockEditor & {
    ByteModifyTools{_data}.replace(range, replacement.dataView(), replacement.isSensitive());
    return *this;
}

auto ByteBlockEditor::replace(const ByteRange range, const ConstByteSpan replacement) -> ByteBlockEditor & {
    ByteModifyTools{_data}.replace(range, impl::ByteDataView{replacement}, false);
    return *this;
}

auto ByteBlockEditor::insert(const ByteIndex index, const ByteBlock &bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.insert(index, bytes.dataView(), bytes.isSensitive());
    return *this;
}

auto ByteBlockEditor::insert(const ByteIndex index, const ConstByteSpan bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.insert(index, impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBlockEditor::append(const Byte value, const ByteLength length) -> ByteBlockEditor & {
    ByteModifyTools{_data}.append(value, length);
    return *this;
}

auto ByteBlockEditor::append(const ByteBlock &bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.append(bytes.dataView(), bytes.isSensitive());
    return *this;
}

auto ByteBlockEditor::append(const ConstByteSpan bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.append(impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBlockEditor::overwrite(const ByteRange range, const ByteBlock &bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.overwrite(range, bytes.dataView(), bytes.isSensitive());
    return *this;
}

auto ByteBlockEditor::overwrite(const ConstByteSpan bytes) -> ByteBlockEditor & {
    return overwrite(ByteRange::all(), bytes);
}

auto ByteBlockEditor::overwrite(const ByteIndex index, const ConstByteSpan bytes) -> ByteBlockEditor & {
    return overwrite(ByteRange{index, ByteLength::infinite()}, bytes);
}

auto ByteBlockEditor::overwrite(const ByteRange range, const ConstByteSpan bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.overwrite(range, impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBlockEditor::fill(const Byte value) -> ByteBlockEditor & {
    return fill(ByteRange::all(), value);
}

auto ByteBlockEditor::fill(const ByteRange range, const Byte value) -> ByteBlockEditor & {
    ByteModifyTools{_data}.fill(range, value);
    return *this;
}

auto ByteBlockEditor::xorWith(const ByteBlock &bytes) -> bool {
    return ByteModifyTools{_data}.xorWith(bytes.dataView(), bytes.isSensitive());
}

auto ByteBlockEditor::xorWithOrThrow(const ByteBlock &bytes) -> ByteBlockEditor & {
    if (!xorWith(bytes)) {
        throw err::ParameterError{"XOR operands must have equal lengths"_el, "bytes"_el};
    }
    return *this;
}

auto ByteBlockEditor::xorWith(const ConstByteSpan bytes) -> bool {
    return ByteModifyTools{_data}.xorWith(impl::ByteDataView{bytes}, false);
}

auto ByteBlockEditor::xorWithOrThrow(const ConstByteSpan bytes) -> ByteBlockEditor & {
    if (!xorWith(bytes)) {
        throw err::ParameterError{"XOR operands must have equal lengths."_el, "bytes"_el};
    }
    return *this;
}

auto ByteBlockEditor::xorWith(const ByteRange range, const ConstByteSpan bytes) -> ByteBlockEditor & {
    ByteModifyTools{_data}.xorWith(range, impl::ByteDataView{bytes}, false);
    return *this;
}

auto ByteBlockEditor::removed(const ByteRange range) const -> ByteBlockEditor {
    auto result = *this;
    result.remove(range);
    return result;
}

auto ByteBlockEditor::kept(const ByteRange range) const -> ByteBlockEditor {
    return ByteBlockEditor{ByteModifyTools<>::createData(span(range), isSensitive())};
}

auto ByteBlockEditor::replaced(const ByteRange range, const ByteBlock &replacement) const -> ByteBlockEditor {
    auto result = *this;
    result.replace(range, replacement);
    return result;
}

auto ByteBlockEditor::join(const std::initializer_list<ByteBlock> parts) const -> ByteBlockEditor {
    if (parts.size() == 0U) {
        return {};
    }
    auto result = ByteBlockEditor{};
    auto isFirst = true;
    for (const auto &part : parts) {
        if (!isFirst) {
            ByteModifyTools{result._data}.append(dataView(), isSensitive());
        }
        result.append(part);
        isFirst = false;
    }
    return result;
}

auto ByteBlockEditor::toByteBuffer() const -> ByteBuffer {
    return ByteBuffer{span()};
}

auto ByteBlockEditor::toUInt8Vector() const -> std::vector<uint8_t> {
    return impl::ByteReadTools{dataView()}.toUInt8Vector();
}

auto ByteBlockEditor::toCharVector() const -> std::vector<char> {
    return impl::ByteReadTools{dataView()}.toCharVector();
}

void ByteBlockEditor::detach() {
    ByteModifyTools{_data}.detach();
}

auto ByteBlockEditor::capacity() const noexcept -> ByteLength {
    return _data.isNull() ? ByteLength::zero() : ByteLength::fromSizeT(_data.constGet()->capacity());
}

void ByteBlockEditor::reserve(const ByteLength capacity) {
    ByteModifyTools{_data}.reserve(capacity);
}

void ByteBlockEditor::shrinkToFit() {
    ByteModifyTools{_data}.shrinkToFit();
}

auto ByteBlockEditor::fromSpan(const ConstByteSpan bytes) -> ByteBlockEditor {
    return ByteBlockEditor{ByteModifyTools<>::createData(bytes)};
}

auto ByteBlockEditor::fromSpan(const std::span<const std::byte> bytes) -> ByteBlockEditor {
    return fromSpan(toConstByteSpan(bytes));
}

auto ByteBlockEditor::fromSpan(const std::span<const uint8_t> bytes) -> ByteBlockEditor {
    return fromSpan(toConstByteSpan(bytes));
}

auto ByteBlockEditor::fromSpan(const std::span<const char> bytes) -> ByteBlockEditor {
    return fromSpan(toConstByteSpan(bytes));
}

auto ByteBlockEditor::fromVector(const std::vector<uint8_t> &bytes) -> ByteBlockEditor {
    return fromSpan(toConstByteSpan(std::span<const uint8_t>{bytes}));
}

auto ByteBlockEditor::fromVector(const std::vector<char> &bytes) -> ByteBlockEditor {
    return fromSpan(toConstByteSpan(std::span<const char>{bytes}));
}

auto ByteBlockEditor::fromJoined(const std::initializer_list<ByteBlock> parts) -> ByteBlockEditor {
    auto result = ByteBlockEditor{};
    for (const auto &part : parts) {
        result.append(part);
    }
    return result;
}

void ByteBlockEditor::ensureUnique() {
    ByteModifyTools{_data}.detach();
}

auto ByteBlockEditor::dataView() const noexcept -> impl::ByteDataView {
    if (_data.isNull()) {
        return {};
    }
    const auto *data = _data.constGet();
    return impl::ByteDataView{
        ConstByteSpan{data->data(), static_cast<std::size_t>(data->size())},
        ByteRange::fromSizeT(static_cast<std::size_t>(data->size()))};
}

auto ByteBlockEditor::writableSpan() -> ByteSpan {
    return ByteModifyTools{_data}.writableData();
}

auto ByteBlockEditor::appendZeroed(const ByteLength length) -> ByteIndex {
    return ByteModifyTools{_data}.appendZeroed(length);
}

}
