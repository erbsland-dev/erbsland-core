// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "U32StringList.hpp"
#include "U32StringLiteral.hpp"
#include "U32StringView.hpp"
#include "U32StringViewList.hpp"

#include "impl/U32Encoding.hpp"
#include "impl/U32StringAppendTools.hpp"
#include "impl/U32StringComparisonTools.hpp"
#include "impl/U32StringData.hpp"
#include "impl/U32StringEncodingTools.hpp"
#include "impl/U32StringModifyTools.hpp"
#include "impl/U32StringReadTools.hpp"
#include "impl/U32StringTransformTools.hpp"
#include "impl/U32Writer.hpp"

#include "../StringConverter.hpp"
#include "../u16/impl/U16Encoding.hpp"
#include "../u16/U16String.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8String.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../util/HashHelper.hpp"

namespace erbsland::text {

U32String::U32String(const std::u32string_view stdString) : _storage{stdString} {
}

U32String::U32String(const U32StringLiteral &literal) : _storage{literal.dataView()} {
}

U32String::U32String(const U32StringView &view) : _storage{view.dataView()} {
}

auto U32String::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U32String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U32String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U32String::isValidUtf32() const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.isValidUtf32();
}

auto U32String::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstOf(characters);
}

auto U32String::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U32String::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U32String::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U32String::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastOf(characters);
}

auto U32String::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U32String::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U32String::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept -> unit::CpIndex {
    return impl::U32StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U32String::find(const U32StringView &text, const CharCompareFn compareFn) const noexcept -> unit::CpIndex {
    return impl::U32StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U32String::find(const U32StringView &text, const unit::CpIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::CpIndex {
    return impl::U32StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U32String::length() const noexcept -> unit::CpLength {
    return impl::U32StringReadTools{dataView()}.length();
}

auto U32String::indexAt(const StringSide side) const noexcept -> unit::CpIndex {
    return side == StringSide::Front ? unit::CpIndex::zero() : unit::CpIndex::end(length());
}

auto U32String::charAt(const StringSide side) const noexcept -> Char {
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

auto U32String::charAt(const unit::CpIndex startIndex) const noexcept -> Char {
    return impl::U32StringReadTools{dataView()}.charAt(startIndex);
}

auto U32String::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U32String::advance(unit::CpIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.advance(index, count);
}

auto U32String::retreat(unit::CpIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U32StringReadTools{dataView()}.retreat(index, count);
}

auto U32String::indexAt(const unit::CpIndex index) const noexcept -> unit::CpIndex {
    return index;
}

auto U32String::toCharIndex(const unit::CpIndex index) const noexcept -> unit::CpIndex {
    return index;
}

auto U32String::slice(const unit::CpRange range) const noexcept -> U32String {
    return withRange(impl::U32StringReadTools{dataView()}.sliceRange(range));
}

auto U32String::slice(const StringSide side, const unit::CpLength length) const noexcept -> U32String {
    if (side == StringSide::Front) {
        return slice(unit::CpRange(unit::CpIndex::zero(), length));
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::CpRange{start, indexAt(StringSide::Back)});
}

auto U32String::slice(const StringSide side) const noexcept -> std::tuple<Char, U32String> {
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

auto U32String::clear() noexcept -> U32String & {
    _storage.clear();
    return *this;
}

auto U32String::append(const U32StringView &text, const unit::ElementCount count) -> U32String & {
    impl::U32StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U32String::append(const Char character, unit::CpLength count) -> U32String & {
    impl::U32StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U32String::remove(const unit::CpRange range) -> U32String & {
    impl::U32StringModifyTools::remove(_storage, range);
    return *this;
}

auto U32String::removeAll(const CharSet &characters) -> U32String & {
    impl::U32StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U32String::removeAll(const U32StringView &text, const CharCompareFn compareFn) -> U32String & {
    impl::U32StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U32String::removeFirst(const U32StringView &text, const CharCompareFn compareFn) -> U32String & {
    impl::U32StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U32String::keep(const unit::CpRange range) -> U32String & {
    impl::U32StringModifyTools::keep(_storage, range);
    return *this;
}

auto U32String::insert(const unit::CpIndex index, const U32StringView &text) -> U32String & {
    impl::U32StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U32String::replace(const unit::CpRange range, const U32StringView &text) -> U32String & {
    impl::U32StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U32String::replaceFirst(const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn)
    -> U32String & {
    impl::U32StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U32String::replaceAll(const CharSet &characters, const Char replacement) -> U32String & {
    impl::U32StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U32String::replaceAll(const CharSet &characters, const U32StringView &replacement) -> U32String & {
    impl::U32StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U32String::replaceAll(const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn)
    -> U32String & {
    impl::U32StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U32String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode) -> U32String & {
    return truncate(maximumWidth, mode, U32StringView{});
}

auto U32String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode, const U32StringView &ellipsis)
    -> U32String & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U32String::removed(const unit::CpRange range) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removed(range)};
}

auto U32String::removedAll(const CharSet &characters) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removedAll(characters)};
}

auto U32String::removedAll(const U32StringView &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U32String::removedFirst(const U32StringView &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U32String::kept(const unit::CpRange range) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.kept(range)};
}

auto U32String::inserted(const unit::CpIndex index, const U32StringView &text) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U32String::replaced(const unit::CpRange range, const U32StringView &text) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U32String::replacedFirst(
    const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn) const -> U32String {
    return U32String{
        impl::U32StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32String::replacedAll(const CharSet &characters, const Char replacement) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U32String::replacedAll(const CharSet &characters, const U32StringView &replacement) const -> U32String {
    return U32String{impl::U32StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U32String::replacedAll(
    const U32StringView &text, const U32StringView &replacement, const CharCompareFn compareFn) const -> U32String {
    return U32String{
        impl::U32StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U32String {
    return truncated(maximumWidth, mode, U32StringView{});
}

auto U32String::truncated(
    const unit::CpLength maximumWidth, const TruncateMode mode, const U32StringView &ellipsis) const -> U32String {
    return U32String{impl::U32StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U32String::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U32String {
    return U32String{impl::U32StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U32String::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U32String {
    return impl::U32StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U32String &first, U32String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U32String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> unit::CpLength {
    return impl::U32StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U32String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32String {
    return impl::U32StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U32String::fromCharacter(const Char character, const unit::CpLength count) -> U32String {
    auto storage = impl::U32StringSharedStorage{};
    impl::U32StringAppendTools{storage}.append(character, count);
    return U32String{std::move(storage)};
}

auto U32String::fromBoolean(const bool value, const BooleanFormat format) -> U32String {
    return StringConverter{format.text(value)}.toU32String();
}

auto U32String::fromByteBlock(const mem::ByteBlockView &bytes, const ByteFormat format) -> U32String {
    auto builder = StringBuilder{StringKind::U32};
    builder.appendByteBlock(bytes, format);
    return builder.takeU32String();
}

auto U32String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U32StringTransformTools{dataView()}.forEach(function);
}

auto U32String::transformed(const TransformCharacterFn function) const -> U32String {
    if (auto result = impl::U32StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U32String{std::move(*result)};
    }
    return *this;
}

void U32String::reset() noexcept {
    _storage = {};
}

auto U32String::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U32String::reserve(const unit::CpLength capacity) {
    _storage.reserve(capacity);
}

void U32String::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U32String::capacity() const noexcept -> unit::CpLength {
    return _storage.capacity();
}

auto U32String::memoryUsage() const noexcept -> unit::ByteLength {
    return _storage.memoryUsage();
}

void U32String::detach() {
    _storage.detach();
}

auto U32String::begin() const noexcept -> const_iterator {
    return U32StringConstIterator{U32StringView{*this}, indexAt(StringSide::Front)};
}

auto U32String::end() const noexcept -> const_iterator {
    return U32StringConstIterator{U32StringView{*this}, indexAt(StringSide::Back)};
}

auto U32String::withRange(const unit::CpRange range) const noexcept -> U32String {
    if (range.isEmpty()) {
        return {};
    }
    return U32String{impl::U32StringSharedStorage{_storage.sharedData(), range}};
}

auto U32String::dataView() const noexcept -> impl::U32StringDataView {
    return _storage.dataView();
}

}
