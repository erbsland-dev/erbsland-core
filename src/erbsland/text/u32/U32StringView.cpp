// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringView.hpp"

#include "U32String.hpp"
#include "U32StringConstIterator.hpp"
#include "U32StringList.hpp"
#include "U32StringLiteral.hpp"
#include "U32StringViewList.hpp"

#include "impl/U32Encoding.hpp"
#include "impl/U32StringComparisonTools.hpp"
#include "impl/U32StringData.hpp"
#include "impl/U32StringEncodingTools.hpp"
#include "impl/U32StringModifyTools.hpp"
#include "impl/U32StringReadTools.hpp"
#include "impl/U32StringTransformTools.hpp"

#include "../u16/U16String.hpp"
#include "../u8/U8String.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

namespace erbsland::text {

U32StringView::U32StringView(const U32String &str) noexcept : _storage{str._storage} {
}

U32StringView::U32StringView(const U32StringLiteral &str) noexcept :
    _storage{impl::U32StringLiteralStorage{str._charPtr, str._size}} {
}

auto U32StringView::copy() const -> U32String {
    return U32String{*this};
}

auto U32StringView::isEmpty() const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.isEmpty();
}

auto U32StringView::isValidUtf32() const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.isValidUtf32();
}

auto U32StringView::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U32StringView::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U32StringView::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstOf(characters);
}

auto U32StringView::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U32StringView::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U32StringView::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U32StringView::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastOf(characters);
}

auto U32StringView::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U32StringView::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U32StringView::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U32StringView::find(const U32StringView &text, const CharCompareFn compareFn) const noexcept -> unit::CpIndex {
    return impl::U32StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U32StringView::find(
    const U32StringView &text, const unit::CpIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::CpIndex {
    return impl::U32StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U32StringView::length() const noexcept -> unit::CpLength {
    return impl::U32StringReadTools{dataView()}.length();
}

auto U32StringView::characterLength() const noexcept -> unit::CpLength {
    return impl::U32StringReadTools{dataView()}.length();
}

auto U32StringView::displayWidth() const noexcept -> int {
    return impl::U32StringReadTools{dataView()}.displayWidth();
}

auto U32StringView::indexAt(const StringSide side) const noexcept -> unit::CpIndex {
    return side == StringSide::Front ? unit::CpIndex::zero() : unit::CpIndex::end(length());
}

auto U32StringView::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::CpIndex::zero());
    }
    auto index = indexAt(StringSide::Back);
    if (!retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U32StringView::charAt(const unit::CpIndex startIndex) const noexcept -> Char {
    return impl::U32StringReadTools{dataView()}.charAt(startIndex);
}

auto U32StringView::readCharAndAdvance(unit::CpIndex &index) const noexcept -> Char {
    return impl::U32StringReadTools{dataView()}.read(index);
}

auto U32StringView::readCharAndRetreat(unit::CpIndex &index) const noexcept -> Char {
    return impl::U32StringReadTools{dataView()}.readAndRetreat(index);
}

auto U32StringView::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U32StringView::advance(unit::CpIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.advance(index, count);
}

auto U32StringView::retreat(unit::CpIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.retreat(index, count);
}

auto U32StringView::indexAt(const unit::CpIndex index) const noexcept -> unit::CpIndex {
    return index;
}

auto U32StringView::toCharIndex(const unit::CpIndex index) const noexcept -> unit::CpIndex {
    return index;
}

auto U32StringView::slice(const unit::CpRange range) const noexcept -> U32StringView {
    return withRange(impl::U32StringReadTools{dataView()}.sliceRange(range));
}

auto U32StringView::slice(const StringSide side, const unit::CpLength length) const noexcept -> U32StringView {
    if (side == StringSide::Front) {
        return slice(unit::CpRange(unit::CpIndex::zero(), length));
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::CpRange{start, indexAt(StringSide::Back)});
}

auto U32StringView::slice(const StringSide side, const unit::CpIndex index) const noexcept -> U32StringView {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(unit::CpRange{unit::CpIndex::zero(), end}) : U32StringView{};
    }
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), index});
    }
    return slice(unit::CpRange{index, end});
}

auto U32StringView::slice(const StringSide side) const noexcept -> std::tuple<Char, U32StringView> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = unit::CpIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(unit::CpRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(unit::CpRange{indexAt(StringSide::Front), start})};
}

auto U32StringView::splitAt(const unit::CpIndex index) const noexcept -> std::pair<U32StringView, U32StringView> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U32StringView::removed(const unit::CpRange range) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removed(range)};
}

auto U32StringView::removedAll(const CharSet &characters) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removedAll(characters)};
}

auto U32StringView::removedAll(const U32StringView &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U32StringView::removedFirst(const U32StringView &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U32StringView::kept(const unit::CpRange range) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.kept(range)};
}

auto U32StringView::inserted(const unit::CpIndex index, const U32StringView &text) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U32StringView::replaced(const unit::CpRange range, const U32StringView &text) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U32StringView::replacedAll(const CharSet &characters, const Char replacement) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U32StringView::replacedAll(const CharSet &characters, const U32StringView &replacement) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U32StringView::replacedAll(
    const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn) const -> U32String {
    return U32String{
        impl::U32StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32StringView::replacedFirst(
    const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn) const -> U32String {
    return U32String{
        impl::U32StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32StringView::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U32String {
    return truncated(maximumWidth, mode, U32StringView{});
}

auto U32StringView::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U32StringView &ellipsis) const -> U32String {
    return U32String{impl::U32StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U32StringView::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U32String {
    return U32String{impl::U32StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U32StringView::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U32String {
    return impl::U32StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U32StringView::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> unit::CpLength {
    return impl::U32StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U32StringView::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32String {
    return impl::U32StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U32StringView::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U32StringTransformTools{dataView()}.forEach(function);
}

auto U32StringView::transformed(const TransformCharacterFn function) const -> U32String {
    if (auto result = impl::U32StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U32String{std::move(*result)};
    }
    return U32String{impl::U32StringSharedStorage{dataView()}};
}

auto U32StringView::begin() const noexcept -> const_iterator {
    return U32StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U32StringView::end() const noexcept -> const_iterator {
    return U32StringConstIterator{*this, indexAt(StringSide::Back)};
}

auto U32StringView::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<impl::U32StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<impl::U32StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U32StringView::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<impl::U32StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<impl::U32StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U32StringView::viewForUnchangedTransform() const -> U32StringView {
    if (isFullStorageRange()) {
        return *this;
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U32StringView{impl::U32StringViewStorage{impl::U32StringSharedStorage{view}}};
}

auto U32StringView::withRange(const unit::CpRange range) const noexcept -> U32StringView {
    if (range.isEmpty()) {
        return {};
    }
    if (const auto *shared = std::get_if<impl::U32StringSharedStorage>(&_storage)) {
        return U32StringView{impl::U32StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<impl::U32StringLiteralStorage>(&_storage)) {
        return U32StringView{impl::U32StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U32StringView::dataView() const noexcept -> impl::U32StringDataView {
    if (const auto *shared = std::get_if<impl::U32StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<impl::U32StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
