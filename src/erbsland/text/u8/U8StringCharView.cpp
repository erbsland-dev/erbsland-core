// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringCharView.hpp"

#include "U8StringView.hpp"

#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringEncodingTools.hpp"
#include "impl/U8StringModifyTools.hpp"
#include "impl/U8StringReadTools.hpp"
#include "impl/U8StringTransformTools.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::text {

auto U8StringCharView::length() const noexcept -> unit::CpLength {
    return impl::U8StringCharReadTool{dataView()}.charLength();
}

auto U8StringCharView::indexAt(const StringSide side) const noexcept -> unit::CpIndex {
    return side == StringSide::Front ? unit::CpIndex::zero() : unit::CpIndex::end(length());
}

auto U8StringCharView::charAt(const StringSide side) const noexcept -> Char {
    const auto fullLength = length();
    if (fullLength.isZero()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::CpIndex::zero());
    }
    return impl::U8StringCharReadTool{dataView()}.charAt(unit::CpIndex::end(fullLength - unit::CpLength::one()));
}

auto U8StringCharView::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U8StringCharReadTool{dataView()}.charAt(index);
}

auto U8StringCharView::slice(const unit::CpRange range) const noexcept -> U8StringView {
    return withRange(impl::U8StringCharReadTool{dataView()}.sliceRange(range));
}

auto U8StringCharView::slice(const StringSide side, const unit::CpLength length) const noexcept -> U8StringView {
    if (side == StringSide::Front) {
        return slice(unit::CpRange(unit::CpIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(unit::CpRange::fromLength(fullLength));
    }
    const auto start = unit::CpIndex::end(fullLength - length);
    return slice(unit::CpRange{start, length});
}

auto U8StringCharView::slice(const StringSide side) const noexcept -> std::tuple<Char, U8StringView> {
    const auto fullLength = length();
    if (fullLength.isZero()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        return {
            charAt(unit::CpIndex::zero()),
            slice(unit::CpRange{unit::CpIndex::one(), fullLength - unit::CpLength::one()})};
    }
    const auto lastIndex = unit::CpIndex::end(fullLength - unit::CpLength::one());
    return {charAt(lastIndex), slice(unit::CpRange{unit::CpIndex::zero(), lastIndex})};
}

auto U8StringCharView::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findFirstOf(characters);
}

auto U8StringCharView::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findFirstOf(characters, start);
}

auto U8StringCharView::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findFirstNotOf(characters);
}

auto U8StringCharView::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findFirstNotOf(characters, start);
}

auto U8StringCharView::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findLastOf(characters);
}

auto U8StringCharView::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findLastOf(characters, end);
}

auto U8StringCharView::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findLastNotOf(characters);
}

auto U8StringCharView::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.findLastNotOf(characters, end);
}

auto U8StringCharView::find(const U8StringView &text, const CharCompareFn compareFn) const noexcept -> unit::CpIndex {
    const auto byteIndex = impl::U8StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
    return impl::U8StringCharReadTool{dataView()}.charIndexAt(byteIndex);
}

auto U8StringCharView::find(
    const U8StringView &text, const unit::CpIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::CpIndex {
    const auto startByteIndex = impl::U8StringCharReadTool{dataView()}.byteIndexAt(start);
    const auto byteIndex = impl::U8StringComparisonTools{dataView()}.find(text.dataView(), startByteIndex, compareFn);
    return impl::U8StringCharReadTool{dataView()}.charIndexAt(byteIndex);
}

auto U8StringCharView::removed(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8StringCharView::removedAll(const CharSet &characters) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedAll(characters)};
}

auto U8StringCharView::removedAll(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U8StringCharView::removedFirst(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U8StringCharView::kept(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8StringCharView::inserted(const unit::CpIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8StringCharView::replaced(const unit::CpRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8StringCharView::replacedAll(const CharSet &characters, const Char replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U8StringCharView::replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U8StringCharView::replacedAll(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringCharView::replacedFirst(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8StringCharView::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U8String {
    return truncated(maximumWidth, mode, U8StringView{});
}

auto U8StringCharView::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis) const -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U8StringCharView::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U8StringCharView::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U8String {
    return impl::U8StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U8StringCharView::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U8StringTransformTools{dataView()}.forEach(function);
}

auto U8StringCharView::transformed(const TransformCharacterFn function) const -> U8String {
    if (auto result = impl::U8StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U8String{std::move(*result)};
    }
    return U8String{impl::U8StringSharedStorage{dataView()}};
}

auto U8StringCharView::withRange(const unit::ByteRange range) const noexcept -> U8StringView {
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

auto U8StringCharView::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U8StringCharView::viewForUnchangedTransform() const -> U8StringView {
    if (isFullStorageRange()) {
        return U8StringView{_storage};
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U8StringView{impl::U8StringViewStorage{impl::U8StringSharedStorage{view}}};
}

auto U8StringCharView::dataView() const noexcept -> impl::U8StringDataView {
    if (const auto *shared = std::get_if<impl::U8StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<impl::U8StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
