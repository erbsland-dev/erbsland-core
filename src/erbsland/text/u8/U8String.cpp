// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "U8StringCharView.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"
#include "U8StringView.hpp"
#include "U8StringViewList.hpp"

#include "impl/U8Encoding.hpp"
#include "impl/U8StringAppendTools.hpp"
#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringData.hpp"
#include "impl/U8StringEncodingTools.hpp"
#include "impl/U8StringModifyTools.hpp"
#include "impl/U8StringReadTools.hpp"
#include "impl/U8StringTransformTools.hpp"
#include "impl/U8Writer.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u16/U16String.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32String.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../util/HashHelper.hpp"

namespace erbsland::text {

U8String::U8String(const std::string_view stdString) : _storage{stdString} {
}

U8String::U8String(const std::u8string_view stdString) : _storage{stdString} {
}

U8String::U8String(const U8StringLiteral<char> &literal) : _storage{literal.dataView()} {
}

U8String::U8String(const U8StringLiteral<char8_t> &literal) : _storage{literal.dataView()} {
}

U8String::U8String(const U8StringView &view) : _storage{view.dataView()} {
}

auto U8String::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U8String::isValidUtf8() const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.isValidUtf8();
}

auto U8String::findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstOf(characters);
}

auto U8String::findFirstOf(const CharSet &characters, const unit::ByteIndex start) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U8String::findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U8String::findFirstNotOf(const CharSet &characters, const unit::ByteIndex start) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U8String::findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastOf(characters);
}

auto U8String::findLastOf(const CharSet &characters, const unit::ByteIndex end) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U8String::findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U8String::findLastNotOf(const CharSet &characters, const unit::ByteIndex end) const noexcept -> unit::ByteIndex {
    return impl::U8StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U8String::find(const U8StringView &text, const CharCompareFn compareFn) const noexcept -> unit::ByteIndex {
    return impl::U8StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U8String::find(const U8StringView &text, const unit::ByteIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::ByteIndex {
    return impl::U8StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U8String::length() const noexcept -> unit::ByteLength {
    return impl::U8StringReadTools{dataView()}.byteLength();
}

auto U8String::characterLength() const noexcept -> unit::CpLength {
    return impl::U8StringCharReadTool{dataView()}.charLength();
}

auto U8String::indexAt(const StringSide side) const noexcept -> unit::ByteIndex {
    return side == StringSide::Front ? unit::ByteIndex::zero() : unit::ByteIndex::end(length());
}

auto U8String::charAt(const StringSide side) const noexcept -> Char {
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

auto U8String::charAt(const unit::ByteIndex startIndex) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.charAt(startIndex);
}

auto U8String::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U8StringCharReadTool{dataView()}.charAt(index);
}

auto U8String::operator[](const unit::ByteIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8String::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8String::advance(unit::ByteIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.advance(index, count);
}

auto U8String::retreat(unit::ByteIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.retreat(index, count);
}

auto U8String::indexAt(const unit::CpIndex index) const noexcept -> unit::ByteIndex {
    return impl::U8StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U8String::toCharIndex(const unit::ByteIndex index) const noexcept -> unit::CpIndex {
    return impl::U8StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U8String::slice(const unit::ByteRange range) const noexcept -> U8String {
    return withRange(impl::U8StringReadTools{dataView()}.sliceRange(range));
}

auto U8String::slice(const unit::CpRange range) const noexcept -> U8String {
    return withRange(impl::U8StringCharReadTool{dataView()}.sliceRange(range));
}

auto U8String::slice(const StringSide side, const unit::ByteLength length) const noexcept -> U8String {
    if (side == StringSide::Front) {
        return slice(unit::ByteRange(unit::ByteIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(unit::ByteRange::fromLength(fullLength));
    }
    const auto start = unit::ByteIndex::fromSizeT(fullLength.toSizeT() - length.toSizeT());
    return slice(unit::ByteRange{start, length});
}

auto U8String::slice(const StringSide side, const unit::CpLength length) const noexcept -> U8String {
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::ByteRange{start, indexAt(StringSide::Back)});
}

auto U8String::slice(const StringSide side) const noexcept -> std::tuple<Char, U8String> {
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

auto U8String::clear() noexcept -> U8String & {
    _storage.clear();
    return *this;
}

auto U8String::append(const U8StringView &text, const unit::ElementCount count) -> U8String & {
    impl::U8StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U8String::append(const Char character, unit::CpLength count) -> U8String & {
    impl::U8StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U8String::remove(const unit::ByteRange range) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8String::remove(const unit::CpRange range) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, range);
    return *this;
}

auto U8String::removeAll(const CharSet &characters) -> U8String & {
    impl::U8StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U8String::removeAll(const U8StringView &text, const CharCompareFn compareFn) -> U8String & {
    impl::U8StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8String::removeFirst(const U8StringView &text, const CharCompareFn compareFn) -> U8String & {
    impl::U8StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U8String::keep(const unit::ByteRange range) -> U8String & {
    impl::U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8String::keep(const unit::CpRange range) -> U8String & {
    impl::U8StringModifyTools::keep(_storage, range);
    return *this;
}

auto U8String::insert(const unit::ByteIndex index, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8String::insert(const unit::CpIndex index, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U8String::replace(const unit::ByteRange range, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8String::replace(const unit::CpRange range, const U8StringView &text) -> U8String & {
    impl::U8StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U8String::replaceFirst(const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn)
    -> U8String & {
    impl::U8StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8String::replaceAll(const CharSet &characters, const Char replacement) -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U8String::replaceAll(const CharSet &characters, const U8StringView &replacement) -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U8String::replaceAll(const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn)
    -> U8String & {
    impl::U8StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U8String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode) -> U8String & {
    return truncate(maximumWidth, mode, U8StringView{});
}

auto U8String::truncate(const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis)
    -> U8String & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U8String::removed(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8String::removed(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(range)};
}

auto U8String::removedAll(const CharSet &characters) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedAll(characters)};
}

auto U8String::removedAll(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U8String::removedFirst(const U8StringView &text, const CharCompareFn compareFn) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U8String::kept(const unit::ByteRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8String::kept(const unit::CpRange range) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.kept(range)};
}

auto U8String::inserted(const unit::ByteIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8String::inserted(const unit::CpIndex index, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U8String::replaced(const unit::ByteRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8String::replaced(const unit::CpRange range, const U8StringView &text) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U8String::replacedFirst(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8String::replacedAll(const CharSet &characters, const Char replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U8String::replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String {
    return U8String{impl::U8StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U8String::replacedAll(
    const U8StringView &text, const U8StringView &replacement, const CharCompareFn compareFn) const -> U8String {
    return U8String{
        impl::U8StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U8String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode) const -> U8String {
    return truncated(maximumWidth, mode, U8StringView{});
}

auto U8String::truncated(const unit::CpLength maximumWidth, const TruncateMode mode, const U8StringView &ellipsis) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U8String::aligned(const unit::CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U8String {
    return U8String{impl::U8StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U8String::toSafeString(const unit::CpLength maximumWidth, const SafeStringFlags flags) const -> U8String {
    return impl::U8StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U8String &first, U8String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U8String::toCharView() const noexcept -> U8StringCharView {
    return U8StringCharView{_storage};
}

void U8String::reset() noexcept {
    _storage = {};
}

auto U8String::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U8String::reserve(const unit::ByteLength capacity) {
    _storage.reserve(capacity);
}

void U8String::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U8String::capacity() const noexcept -> unit::ByteLength {
    return _storage.capacity();
}

auto U8String::memoryUsage() const noexcept -> unit::ByteLength {
    return _storage.memoryUsage();
}

auto U8String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> unit::ByteLength {
    return impl::U8StringTransformTools{_storage.dataView()}.escapedSize(format, amount);
}

auto U8String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8String {
    return impl::U8StringTransformTools{_storage.dataView()}.toEscaped(format, amount);
}

auto U8String::fromCharacter(const Char character, const unit::CpLength count) -> U8String {
    auto storage = impl::U8StringSharedStorage{};
    impl::U8StringAppendTools{storage}.append(character, count);
    return U8String{std::move(storage)};
}

auto U8String::fromBoolean(const bool value, const BooleanFormat format) -> U8String {
    return U8String{format.text(value)};
}

auto U8String::fromByteBlock(const mem::ByteBlockView &bytes, const ByteFormat format) -> U8String {
    auto builder = StringBuilder{};
    builder.appendByteBlock(bytes, format);
    return builder.takeU8String();
}

auto U8String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U8String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U8String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return impl::U8StringTransformTools{dataView()}.forEach(function);
}

auto U8String::transformed(const TransformCharacterFn function) const -> U8String {
    if (auto result = impl::U8StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U8String{std::move(*result)};
    }
    return *this;
}

void U8String::detach() {
    _storage.detach();
}

auto U8String::begin() const noexcept -> const_iterator {
    return U8StringConstIterator{U8StringView{*this}, indexAt(StringSide::Front)};
}

auto U8String::end() const noexcept -> const_iterator {
    return U8StringConstIterator{U8StringView{*this}, indexAt(StringSide::Back)};
}

auto U8String::withRange(const unit::ByteRange range) const noexcept -> U8String {
    if (range.isEmpty()) {
        return {};
    }
    return U8String{impl::U8StringSharedStorage{_storage.sharedData(), range}};
}

auto U8String::dataView() const noexcept -> impl::U8StringDataView {
    return _storage.dataView();
}

}
