// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "U16StringConstIterator.hpp"
#include "U16StringEditor.hpp"
#include "U16StringEditorList.hpp"
#include "U16StringList.hpp"
#include "U16StringLiteral.hpp"

#include "impl/U16Encoding.hpp"
#include "impl/U16StringAppendTools.hpp"
#include "impl/U16StringCharReadTool.hpp"
#include "impl/U16StringComparisonTools.hpp"
#include "impl/U16StringData.hpp"
#include "impl/U16StringEncodingTools.hpp"
#include "impl/U16StringModifyTools.hpp"
#include "impl/U16StringReadTools.hpp"
#include "impl/U16StringTransformTools.hpp"

#include "../impl/ByteBlockFormatter.hpp"
#include "../impl/FloatConversion.hpp"
#include "../StringConverter.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using namespace impl;
using namespace unit;

U16String::U16String(const std::u16string_view stdString) : _storage{U16StringSharedStorage{stdString}} {
}

U16String::U16String(const U16StringEditor &str) noexcept : _storage{str._storage} {
}

U16String::U16String(const U16StringLiteral &str) noexcept :
    _storage{U16StringLiteralStorage{str._charPtr, str._size}} {
}

auto U16String::copy() const -> U16String {
    return U16String{U16StringEditor{*this}};
}

auto U16String::isEmpty() const noexcept -> bool {
    return U16StringReadTools{dataView()}.isEmpty();
}

auto U16String::isValidUtf16() const noexcept -> bool {
    return U16StringReadTools{dataView()}.isValidUtf16();
}

auto U16String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U16String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U16String::findFirstOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstOf(characters);
}

auto U16String::findFirstOf(const CharSet &characters, const U16DataIndex start) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U16String::findFirstNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U16String::findFirstNotOf(const CharSet &characters, const U16DataIndex start) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U16String::findLastOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastOf(characters);
}

auto U16String::findLastOf(const CharSet &characters, const U16DataIndex end) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U16String::findLastNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U16String::findLastNotOf(const CharSet &characters, const U16DataIndex end) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U16String::find(const U16String &text, const CharCompareFn compareFn) const noexcept -> U16DataIndex {
    return U16StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U16String::find(const U16String &text, const U16DataIndex start, const CharCompareFn compareFn) const noexcept
    -> U16DataIndex {
    return U16StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U16String::length() const noexcept -> U16DataLength {
    return U16StringReadTools{dataView()}.byteLength();
}

auto U16String::characterLength() const noexcept -> CpLength {
    return U16StringCharReadTool{dataView()}.charLength();
}

auto U16String::displayWidth() const noexcept -> int {
    return U16StringReadTools{dataView()}.displayWidth();
}

auto U16String::indexAt(const StringSide side) const noexcept -> U16DataIndex {
    return side == StringSide::Front ? U16DataIndex::zero() : U16DataIndex::end(length());
}

auto U16String::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(U16DataIndex::zero());
    }
    auto index = indexAt(StringSide::Back);
    if (!retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U16String::charAt(const U16DataIndex startIndex) const noexcept -> Char {
    return U16StringReadTools{dataView()}.charAt(startIndex);
}

auto U16String::readCharAndAdvance(U16DataIndex &index) const noexcept -> Char {
    return U16StringReadTools{dataView()}.read(index);
}

auto U16String::readCharAndRetreat(U16DataIndex &index) const noexcept -> Char {
    return U16StringReadTools{dataView()}.readAndRetreat(index);
}

auto U16String::charAt(const CpIndex index) const noexcept -> Char {
    return U16StringCharReadTool{dataView()}.charAt(index);
}

auto U16String::operator[](const U16DataIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16String::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16String::advance(U16DataIndex &index, const CpLength count) const noexcept -> bool {
    return U16StringReadTools{dataView()}.advance(index, count);
}

auto U16String::retreat(U16DataIndex &index, const CpLength count) const noexcept -> bool {
    return U16StringReadTools{dataView()}.retreat(index, count);
}

auto U16String::indexAt(const CpIndex index) const noexcept -> U16DataIndex {
    return U16StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U16String::toCharIndex(const U16DataIndex index) const noexcept -> CpIndex {
    return U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16String::slice(const U16DataRange range) const noexcept -> U16String {
    return withRange(U16StringReadTools{dataView()}.sliceRange(range));
}

auto U16String::slice(const CpRange range) const noexcept -> U16String {
    return withRange(U16StringCharReadTool{dataView()}.sliceRange(range));
}

auto U16String::slice(const StringSide side, const U16DataLength length) const noexcept -> U16String {
    if (side == StringSide::Front) {
        return slice(U16DataRange(U16DataIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(U16DataRange::fromLength(fullLength));
    }
    const auto start = U16DataIndex::end(fullLength - length);
    return slice(U16DataRange{start, length});
}

auto U16String::slice(const StringSide side, const U16DataIndex index) const noexcept -> U16String {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(U16DataRange{U16DataIndex::zero(), end}) : U16String{};
    }
    if (side == StringSide::Front) {
        return slice(U16DataRange{U16DataIndex::zero(), index});
    }
    return slice(U16DataRange{index, end});
}

auto U16String::slice(const StringSide side, const CpLength length) const noexcept -> U16String {
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(U16DataRange{start, indexAt(StringSide::Back)});
}

auto U16String::slice(const StringSide side, const CpIndex index) const noexcept -> U16String {
    return slice(side, indexAt(index));
}

auto U16String::slice(const StringSide side) const noexcept -> std::tuple<Char, U16String> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = U16DataIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(U16DataRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(U16DataRange{indexAt(StringSide::Front), start})};
}

auto U16String::splitAt(const U16DataIndex index) const noexcept -> std::pair<U16String, U16String> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U16String::splitAt(const CpIndex index) const noexcept -> std::pair<U16String, U16String> {
    return splitAt(indexAt(index));
}

auto U16String::removed(const U16DataRange range) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.removed(range)};
}

auto U16String::removed(const CpRange range) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.removed(range)};
}

auto U16String::removedAll(const CharSet &characters) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.removedAll(characters)};
}

auto U16String::removedAll(const U16String &text, const CharCompareFn compareFn) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U16String::removedFirst(const U16String &text, const CharCompareFn compareFn) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U16String::kept(const U16DataRange range) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::kept(const CpRange range) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.kept(range)};
}

auto U16String::inserted(const U16DataIndex index, const U16String &text) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16String::inserted(const CpIndex index, const U16String &text) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U16String::replaced(const U16DataRange range, const U16String &text) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16String::replaced(const CpRange range, const U16String &text) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U16String::replacedAll(const CharSet &characters, const Char replacement) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U16String::replacedAll(const CharSet &characters, const U16String &replacement) const -> U16String {
    return U16StringEditor{U16StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U16String::replacedAll(const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const
    -> U16String {
    return U16StringEditor{
        U16StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16String::replacedFirst(const U16String &text, const U16String &replacement, const CharCompareFn compareFn) const
    -> U16String {
    return U16StringEditor{
        U16StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U16String::truncated(const CpLength maximumWidth, const TruncateMode mode) const -> U16String {
    return truncated(maximumWidth, mode, U16String{});
}

auto U16String::truncated(const CpLength maximumWidth, const TruncateMode mode, const U16String &ellipsis) const
    -> U16String {
    return U16StringEditor{U16StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U16String::aligned(const CpLength length, const bgeo::Alignment alignment, const Char fill) const -> U16String {
    return U16StringEditor{U16StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U16String::toSafeString(const CpLength maximumWidth, const SafeStringFlags flags) const -> U16String {
    return U16StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

auto U16String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> U16DataLength {
    return U16StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U16String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U16String {
    return U16StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U16String::fromCharacter(const Char character, const CpLength count) -> U16String {
    auto storage = U16StringSharedStorage{};
    U16StringAppendTools{storage}.append(character, count);
    return U16String{U16StringStorage{std::move(storage)}};
}

auto U16String::fromJoined(const std::initializer_list<U16String> parts) -> U16String {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = U16StringSharedStorage::checkedAddSize(
            finalSize, part.length().toSizeT(), "Joined string exceeds size bounds");
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = U16StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size() * sizeof(char16_t));
        writePosition = U16StringSharedStorage::checkedAddSize(
            writePosition, data.size(), "Joined string write exceeds size bounds");
    }
    return U16String{U16StringStorage{std::move(storage)}};
}

auto U16String::fromFloat(const double value, const FloatFormat format) -> U16String {
    return StringConverter{impl::formatFloat(value, format)}.toU16String();
}

auto U16String::fromBoolean(const bool value, const BooleanFormat format) -> U16String {
    return StringConverter{format.text(value)}.toU16String();
}

auto U16String::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U16String {
    auto storage = U16StringSharedStorage{};
    auto appendTools = U16StringAppendTools{storage};
    impl::formatByteBlock(appendTools, bytes, format);
    return U16String{U16StringStorage{std::move(storage)}};
}

auto U16String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U16StringTransformTools{dataView()}.forEach(function);
}

auto U16String::transformed(const TransformCharacterFn function) const -> U16String {
    if (auto result = U16StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U16StringEditor{std::move(*result)};
    }
    return U16StringEditor{U16StringSharedStorage{dataView()}};
}

auto U16String::begin() const noexcept -> const_iterator {
    return U16StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U16String::end() const noexcept -> const_iterator {
    return U16StringConstIterator{*this, indexAt(StringSide::Back)};
}

void swap(U16String &first, U16String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U16String::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<U16StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<U16StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U16String::isFullStorageRange() const noexcept -> bool {
    if (const auto *shared = std::get_if<U16StringSharedStorage>(&_storage)) {
        return shared->range().index().isZero() && shared->range().length().toSizeT() == shared->dataSize();
    }
    if (const auto *literal = std::get_if<U16StringLiteralStorage>(&_storage)) {
        return literal->range().index().isZero() && literal->range().length().toSizeT() == literal->size();
    }
    return false;
}

auto U16String::stringForUnchangedTransform() const -> U16String {
    if (isFullStorageRange()) {
        return *this;
    }
    const auto view = dataView();
    if (view.dataSpan().empty()) {
        return {};
    }
    return U16String{U16StringStorage{U16StringSharedStorage{view}}};
}

auto U16String::withRange(const U16DataRange range) const noexcept -> U16String {
    if (range.isEmpty()) {
        return {};
    }
    if (const auto *shared = std::get_if<U16StringSharedStorage>(&_storage)) {
        return U16String{U16StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<U16StringLiteralStorage>(&_storage)) {
        return U16String{U16StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U16String::dataView() const noexcept -> U16StringDataView {
    if (const auto *shared = std::get_if<U16StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<U16StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

}
