// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringCharView.hpp"

#include "U16StringView.hpp"

#include "impl/U16StringCharReadTool.hpp"
#include "impl/U16StringComparisonTools.hpp"
#include "impl/U16StringEncodingTools.hpp"
#include "impl/U16StringModifyTools.hpp"
#include "impl/U16StringReadTools.hpp"
#include "impl/U16StringTransformTools.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::text {

auto U16StringCharView::length() const noexcept -> unit::CpLength {
    return impl::U16StringCharReadTool{dataView()}.charLength();
}

auto U16StringCharView::indexAt(const StringSide side) const noexcept -> unit::CpIndex {
    return side == StringSide::Front ? unit::CpIndex::zero() : unit::CpIndex::end(length());
}

auto U16StringCharView::charAt(const StringSide side) const noexcept -> Char {
    const auto fullLength = length();
    if (fullLength.isZero()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::CpIndex::zero());
    }
    return charAt(unit::CpIndex::end(fullLength - unit::CpLength::one()));
}

auto U16StringCharView::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U16StringCharReadTool{dataView()}.charAt(index);
}

auto U16StringCharView::slice(const unit::CpRange range) const noexcept -> U16StringView {
    return withRange(impl::U16StringCharReadTool{dataView()}.sliceRange(range));
}

auto U16StringCharView::slice(const StringSide side, const unit::CpLength length) const noexcept -> U16StringView {
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

auto U16StringCharView::slice(const StringSide side) const noexcept -> std::tuple<Char, U16StringView> {
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

auto U16StringCharView::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findFirstOf(characters);
}

auto U16StringCharView::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findFirstOf(characters, start);
}

auto U16StringCharView::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findFirstNotOf(characters);
}

auto U16StringCharView::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findFirstNotOf(characters, start);
}

auto U16StringCharView::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findLastOf(characters);
}

auto U16StringCharView::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findLastOf(characters, end);
}

auto U16StringCharView::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findLastNotOf(characters);
}

auto U16StringCharView::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.findLastNotOf(characters, end);
}

auto U16StringCharView::find(const U16StringView &text, const CharCompareFn compareFn) const noexcept -> unit::CpIndex {
    const auto index = impl::U16StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
    return impl::U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16StringCharView::find(
    const U16StringView &text, const unit::CpIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::CpIndex {
    const auto dataIndex = impl::U16StringCharReadTool{dataView()}.byteIndexAt(start);
    const auto index = impl::U16StringComparisonTools{dataView()}.find(text.dataView(), dataIndex, compareFn);
    return impl::U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16StringCharView::removed(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(range)};
}

auto U16StringCharView::removedAll(const CharSet &characters) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedAll(characters)};
}

auto U16StringCharView::removedAll(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U16StringCharView::removedFirst(const U16StringView &text, const CharCompareFn compareFn) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U16StringCharView::kept(const unit::CpRange range) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.kept(range)};
}

auto U16StringCharView::inserted(const unit::CpIndex index, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16StringCharView::replaced(const unit::CpRange range, const U16StringView &text) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16StringCharView::replacedAll(const CharSet &characters, const Char replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U16StringCharView::replacedAll(const CharSet &characters, const U16StringView &replacement) const -> U16String {
    return U16String{impl::U16StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U16StringCharView::replacedAll(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16StringCharView::replacedFirst(
    const U16StringView &text, const U16StringView &replacement, const CharCompareFn compareFn) const -> U16String {
    return U16String{
        impl::U16StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16StringCharView::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U16String {
    return truncated(maximumWidth, mode, U16StringView{});
}

auto U16StringCharView::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U16StringView &ellipsis) const -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U16StringCharView::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U16String {
    return U16String{impl::U16StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U16StringCharView::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const
    -> U16String {
    return impl::U16StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U16StringCharView::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U16StringTransformTools{dataView()}.forEach(function);
}

auto U16StringCharView::transformed(const TransformCharacterFn function) const -> U16String {
    if (auto result = impl::U16StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U16String{std::move(*result)};
    }
    return U16String{impl::U16StringSharedStorage{dataView()}};
}

auto U16StringCharView::withRange(const unit::U16DataRange range) const noexcept -> U16StringView {
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

auto U16StringCharView::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U16StringCharView::viewForUnchangedTransform() const -> U16StringView {
    if (isFullStorageRange()) {
        return U16StringView{_storage};
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U16StringView{impl::U16StringViewStorage{impl::U16StringSharedStorage{view}}};
}

auto U16StringCharView::dataView() const noexcept -> impl::U16StringDataView {
    if (const auto *shared = std::get_if<impl::U16StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<impl::U16StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
