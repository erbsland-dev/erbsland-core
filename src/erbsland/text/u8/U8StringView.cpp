// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringView.hpp"

#include "U8String.hpp"
#include "U8StringCharView.hpp"
#include "U8StringConstIterator.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"
#include "U8StringViewList.hpp"

#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringData.hpp"
#include "impl/U8StringEncodingTools.hpp"
#include "impl/U8StringModifyTools.hpp"
#include "impl/U8StringReadTools.hpp"
#include "impl/U8StringTransformTools.hpp"

#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

namespace erbsland::text {

U8StringView::U8StringView(const U8String &str) noexcept : _storage{str._storage} {
}

U8StringView::U8StringView(const U8StringLiteral<char> &str) noexcept :
    _storage{impl::U8StringLiteralStorage{str._charPtr, str._size}} {
}

U8StringView::U8StringView(const U8StringLiteral<char8_t> &str) noexcept :
    _storage{impl::U8StringLiteralStorage{str._charPtr, str._size}} {
}

auto U8StringView::copy() const -> U8String {
    return U8String{*this};
}

auto U8StringView::isEmpty() const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.isEmpty();
}

auto U8StringView::isValidUtf8() const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.isValidUtf8();
}

auto U8StringView::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U8StringView::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U8StringView::findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstOf(characters);
}

auto U8StringView::findFirstOf(const CharSet &characters, const unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U8StringView::findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U8StringView::findFirstNotOf(const CharSet &characters, const unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U8StringView::findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastOf(characters);
}

auto U8StringView::findLastOf(const CharSet &characters, const unit::ByteIndex end) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U8StringView::findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U8StringView::findLastNotOf(const CharSet &characters, const unit::ByteIndex end) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U8StringView::find(const U8StringView &text, const CharCompareFn compareFn) const noexcept -> unit::ByteIndex {
    return impl::U8StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U8StringView::find(
    const U8StringView &text, const unit::ByteIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U8StringView::length() const noexcept -> unit::ByteLength {
    return impl::U8StringReadTools{dataView()}.byteLength();
}

auto U8StringView::characterLength() const noexcept -> unit::CpLength {
    return impl::U8StringCharReadTool{dataView()}.charLength();
}

auto U8StringView::displayWidth() const noexcept -> int {
    return impl::U8StringReadTools{dataView()}.displayWidth();
}

auto U8StringView::indexAt(const StringSide side) const noexcept -> unit::ByteIndex {
    return side == StringSide::Front ? unit::ByteIndex::zero() : unit::ByteIndex::end(length());
}

auto U8StringView::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::ByteIndex::zero());
    }
    auto index = unit::ByteIndex::end(length());
    if (!impl::U8StringReadTools{dataView()}.retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U8StringView::charAt(const unit::ByteIndex startIndex) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.charAt(startIndex);
}

auto U8StringView::readCharAndAdvance(unit::ByteIndex &index) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.read(index);
}

auto U8StringView::readCharAndRetreat(unit::ByteIndex &index) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.readAndRetreat(index);
}

auto U8StringView::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U8StringCharReadTool{dataView()}.charAt(index);
}

auto U8StringView::operator[](const unit::ByteIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8StringView::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8StringView::advance(unit::ByteIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.advance(index, count);
}

auto U8StringView::retreat(unit::ByteIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.retreat(index, count);
}

auto U8StringView::indexAt(const unit::CpIndex index) const noexcept -> unit::ByteIndex {
    return impl::U8StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U8StringView::toCharIndex(const unit::ByteIndex index) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U8StringView::slice(const unit::ByteRange range) const noexcept -> U8StringView {
    return withRange(impl::U8StringReadTools{dataView()}.sliceRange(range));
}

auto U8StringView::slice(const unit::CpRange range) const noexcept -> U8StringView {
    return withRange(impl::U8StringCharReadTool{dataView()}.sliceRange(range));
}

auto U8StringView::slice(const StringSide side, const unit::ByteLength length) const noexcept -> U8StringView {
    if (side == StringSide::Front) {
        return slice(unit::ByteRange(unit::ByteIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(unit::ByteRange::fromLength(fullLength));
    }
    const auto start = unit::ByteIndex::end(fullLength - length);
    return slice(unit::ByteRange{start, length});
}

auto U8StringView::slice(const StringSide side, const unit::ByteIndex index) const noexcept -> U8StringView {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(unit::ByteRange{unit::ByteIndex::zero(), end}) : U8StringView{};
    }
    if (side == StringSide::Front) {
        return slice(unit::ByteRange{unit::ByteIndex::zero(), index});
    }
    return slice(unit::ByteRange{index, end});
}

auto U8StringView::slice(const StringSide side, const unit::CpLength length) const noexcept -> U8StringView {
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::ByteRange{start, indexAt(StringSide::Back)});
}

auto U8StringView::slice(const StringSide side, const unit::CpIndex index) const noexcept -> U8StringView {
    return slice(side, indexAt(index));
}

auto U8StringView::slice(const StringSide side) const noexcept -> std::tuple<Char, U8StringView> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = unit::ByteIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(unit::ByteRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(unit::ByteRange{indexAt(StringSide::Front), start})};
}

auto U8StringView::splitAt(const unit::ByteIndex index) const noexcept -> std::pair<U8StringView, U8StringView> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U8StringView::splitAt(const unit::CpIndex index) const noexcept -> std::pair<U8StringView, U8StringView> {
    return splitAt(indexAt(index));
}

auto U8StringView::removed(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8StringView::removed(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8StringView::removedAll(const CharSet &characters) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedAll(characters)};
}

auto U8StringView::removedAll(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U8StringView::removedFirst(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U8StringView::kept(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8StringView::kept(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8StringView::inserted(const unit::ByteIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8StringView::inserted(const unit::CpIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8StringView::replaced(const unit::ByteRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8StringView::replaced(const unit::CpRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8StringView::replacedAll(const CharSet &characters, const Char replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U8StringView::replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U8StringView::replacedAll(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringView::replacedFirst(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringView::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U8String {
    return truncated(maximumWidth, mode, U8StringView{});
}

auto U8StringView::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis) const -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U8StringView::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U8StringView::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U8String {
    return impl::U8StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U8StringView::toCharView() const noexcept -> U8StringCharView {
    return U8StringCharView{_storage};
}

auto U8StringView::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> unit::ByteLength {
    return impl::U8StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U8StringView::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8String {
    return impl::U8StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U8StringView::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U8StringTransformTools{dataView()}.forEach(function);
}

auto U8StringView::transformed(const TransformCharacterFn function) const -> U8String {
    if (auto result = impl::U8StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U8String{std::move(*result)};
    }
    return U8String{impl::U8StringSharedStorage{dataView()}};
}

auto U8StringView::begin() const noexcept -> const_iterator {
    return U8StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U8StringView::end() const noexcept -> const_iterator {
    return U8StringConstIterator{*this, indexAt(StringSide::Back)};
}

auto U8StringView::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U8StringView::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U8StringView::viewForUnchangedTransform() const -> U8StringView {
    if (isFullStorageRange()) {
        return *this;
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U8StringView{impl::U8StringViewStorage{impl::U8StringSharedStorage{view}}};
}

auto U8StringView::withRange(const unit::ByteRange range) const noexcept -> U8StringView {
    if (range.isEmpty()) {
        return {};
    }
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return U8StringView{impl::U8StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return U8StringView{impl::U8StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U8StringView::dataView() const noexcept -> impl::U8StringDataView {
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
