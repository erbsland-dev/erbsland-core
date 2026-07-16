// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringView.hpp"

#include "U16String.hpp"
#include "U16StringCharView.hpp"
#include "U16StringConstIterator.hpp"
#include "U16StringList.hpp"
#include "U16StringLiteral.hpp"
#include "U16StringViewList.hpp"

#include "impl/U16Encoding.hpp"
#include "impl/U16StringCharReadTool.hpp"
#include "impl/U16StringComparisonTools.hpp"
#include "impl/U16StringData.hpp"
#include "impl/U16StringEncodingTools.hpp"
#include "impl/U16StringModifyTools.hpp"
#include "impl/U16StringReadTools.hpp"
#include "impl/U16StringTransformTools.hpp"

#include "../u32/U32String.hpp"
#include "../u8/U8String.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

namespace erbsland::text {

U16StringView::U16StringView(const U16String &str) noexcept : _storage{str._storage} {
}

U16StringView::U16StringView(const U16StringLiteral &str) noexcept :
    _storage{impl::U16StringLiteralStorage{str._charPtr, str._size}} {
}

auto U16StringView::copy() const -> U16String {
    return U16String{*this};
}

auto U16StringView::isEmpty() const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.isEmpty();
}

auto U16StringView::isValidUtf16() const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.isValidUtf16();
}

auto U16StringView::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U16StringView::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U16StringView::findFirstOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstOf(characters);
}

auto U16StringView::findFirstOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U16StringView::findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U16StringView::findFirstNotOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U16StringView::findLastOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastOf(characters);
}

auto U16StringView::findLastOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U16StringView::findLastNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U16StringView::findLastNotOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U16StringView::find(const U16StringView &text, const CharCompareFn compareFn) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U16StringView::find(
    const U16StringView &text, const unit::U16DataIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U16StringView::length() const noexcept -> unit::U16DataLength {
    return impl::U16StringReadTools{dataView()}.byteLength();
}

auto U16StringView::characterLength() const noexcept -> unit::CpLength {
    return impl::U16StringCharReadTool{dataView()}.charLength();
}

auto U16StringView::displayWidth() const noexcept -> int {
    return impl::U16StringReadTools{dataView()}.displayWidth();
}

auto U16StringView::indexAt(const StringSide side) const noexcept -> unit::U16DataIndex {
    return side == StringSide::Front ? unit::U16DataIndex::zero() : unit::U16DataIndex::end(length());
}

auto U16StringView::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::U16DataIndex::zero());
    }
    auto index = indexAt(StringSide::Back);
    if (!retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U16StringView::charAt(const unit::U16DataIndex startIndex) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.charAt(startIndex);
}

auto U16StringView::readCharAndAdvance(unit::U16DataIndex &index) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.read(index);
}

auto U16StringView::readCharAndRetreat(unit::U16DataIndex &index) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.readAndRetreat(index);
}

auto U16StringView::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U16StringCharReadTool{dataView()}.charAt(index);
}

auto U16StringView::operator[](const unit::U16DataIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16StringView::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16StringView::advance(unit::U16DataIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.advance(index, count);
}

auto U16StringView::retreat(unit::U16DataIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.retreat(index, count);
}

auto U16StringView::indexAt(const unit::CpIndex index) const noexcept -> unit::U16DataIndex {
    return impl::U16StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U16StringView::toCharIndex(const unit::U16DataIndex index) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16StringView::slice(const unit::U16DataRange range) const noexcept -> U16StringView {
    return withRange(impl::U16StringReadTools{dataView()}.sliceRange(range));
}

auto U16StringView::slice(const unit::CpRange range) const noexcept -> U16StringView {
    return withRange(impl::U16StringCharReadTool{dataView()}.sliceRange(range));
}

auto U16StringView::slice(const StringSide side, const unit::U16DataLength length) const noexcept -> U16StringView {
    if (side == StringSide::Front) {
        return slice(unit::U16DataRange(unit::U16DataIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(unit::U16DataRange::fromLength(fullLength));
    }
    const auto start = unit::U16DataIndex::end(fullLength - length);
    return slice(unit::U16DataRange{start, length});
}

auto U16StringView::slice(const StringSide side, const unit::U16DataIndex index) const noexcept -> U16StringView {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(unit::U16DataRange{unit::U16DataIndex::zero(), end}) : U16StringView{};
    }
    if (side == StringSide::Front) {
        return slice(unit::U16DataRange{unit::U16DataIndex::zero(), index});
    }
    return slice(unit::U16DataRange{index, end});
}

auto U16StringView::slice(const StringSide side, const unit::CpLength length) const noexcept -> U16StringView {
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::U16DataRange{start, indexAt(StringSide::Back)});
}

auto U16StringView::slice(const StringSide side, const unit::CpIndex index) const noexcept -> U16StringView {
    return slice(side, indexAt(index));
}

auto U16StringView::slice(const StringSide side) const noexcept -> std::tuple<Char, U16StringView> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = unit::U16DataIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(unit::U16DataRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(unit::U16DataRange{indexAt(StringSide::Front), start})};
}

auto U16StringView::splitAt(const unit::U16DataIndex index) const noexcept -> std::pair<U16StringView, U16StringView> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U16StringView::splitAt(const unit::CpIndex index) const noexcept -> std::pair<U16StringView, U16StringView> {
    return splitAt(indexAt(index));
}

auto U16StringView::removed(const unit::U16DataRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(range)};
}

auto U16StringView::removed(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(range)};
}

auto U16StringView::removedAll(const CharSet &characters) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedAll(characters)};
}

auto U16StringView::removedAll(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U16StringView::removedFirst(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U16StringView::kept(const unit::U16DataRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.kept(range)};
}

auto U16StringView::kept(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.kept(range)};
}

auto U16StringView::inserted(const unit::U16DataIndex index, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16StringView::inserted(const unit::CpIndex index, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16StringView::replaced(const unit::U16DataRange range, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16StringView::replaced(const unit::CpRange range, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16StringView::replacedAll(const CharSet &characters, const Char replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U16StringView::replacedAll(const CharSet &characters, const U16StringView &replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U16StringView::replacedAll(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16StringView::replacedFirst(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16StringView::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U16String {
    return truncated(maximumWidth, mode, U16StringView{});
}

auto U16StringView::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U16StringView &ellipsis) const -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U16StringView::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U16StringView::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U16String {
    return impl::U16StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U16StringView::toCharView() const noexcept -> U16StringCharView {
    return U16StringCharView{_storage};
}

auto U16StringView::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept
    -> unit::U16DataLength {
    return impl::U16StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U16StringView::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16String {
    return impl::U16StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U16StringView::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U16StringTransformTools{dataView()}.forEach(function);
}

auto U16StringView::transformed(const TransformCharacterFn function) const -> U16String {
    if (auto result = impl::U16StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U16String{std::move(*result)};
    }
    return U16String{impl::U16StringSharedStorage{dataView()}};
}

auto U16StringView::begin() const noexcept -> const_iterator {
    return U16StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U16StringView::end() const noexcept -> const_iterator {
    return U16StringConstIterator{*this, indexAt(StringSide::Back)};
}

auto U16StringView::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U16StringView::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U16StringView::viewForUnchangedTransform() const -> U16StringView {
    if (isFullStorageRange()) {
        return *this;
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U16StringView{impl::U16StringViewStorage{impl::U16StringSharedStorage{view}}};
}

auto U16StringView::withRange(const unit::U16DataRange range) const noexcept -> U16StringView {
    if (range.isEmpty()) {
        return {};
    }
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return U16StringView{impl::U16StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return U16StringView{impl::U16StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U16StringView::dataView() const noexcept -> impl::U16StringDataView {
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
