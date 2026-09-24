// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "U32StringConstIterator.hpp"
#include "U32StringEditor.hpp"
#include "U32StringEditorList.hpp"
#include "U32StringList.hpp"
#include "U32StringLiteral.hpp"

#include "impl/U32Encoding.hpp"
#include "impl/U32StringAppendTools.hpp"
#include "impl/U32StringComparisonTools.hpp"
#include "impl/U32StringData.hpp"
#include "impl/U32StringEncodingTools.hpp"
#include "impl/U32StringModifyTools.hpp"
#include "impl/U32StringReadTools.hpp"
#include "impl/U32StringTransformTools.hpp"

#include "../EncodingMode.hpp"
#include "../impl/ByteBlockFormatter.hpp"
#include "../impl/FloatConversion.hpp"
#include "../impl/StringSafeTransformTools.hpp"
#include "../StringConverter.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using namespace impl;
using unit::CpIndex;
using unit::CpLength;
using unit::CpRange;

U32String::U32String(const std::u32string_view stdString) : _storage{U32StringSharedStorage{stdString}} {
}

U32String::U32String(const U32StringEditor &str) noexcept : _storage{str._storage} {
}

U32String::U32String(const U32StringLiteral &str) noexcept :
    _storage{U32StringLiteralStorage{str._charPtr, str._size}} {
}

auto U32String::copy() const -> U32String {
    return U32String{U32StringSharedStorage{dataView()}};
}

auto U32String::isEmpty() const noexcept -> bool {
    return U32StringReadTools{dataView()}.isEmpty();
}

auto U32String::isValidUtf32() const noexcept -> bool {
    return U32StringReadTools{dataView()}.isValidUtf32();
}

auto U32String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U32String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U32String::findFirstOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstOf(characters);
}

auto U32String::findFirstOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U32String::findFirstNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U32String::findFirstNotOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U32String::findLastOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastOf(characters);
}

auto U32String::findLastOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U32String::findLastNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U32String::findLastNotOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U32String::find(const U32String &text, const CharCompareFn compareFn) const noexcept -> CpIndex {
    return U32StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U32String::find(const U32String &text, const CpIndex start, const CharCompareFn compareFn) const noexcept
    -> CpIndex {
    return U32StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U32String::length() const noexcept -> CpLength {
    return U32StringReadTools{dataView()}.length();
}

auto U32String::characterLength() const noexcept -> CpLength {
    return U32StringReadTools{dataView()}.length();
}

auto U32String::displayWidth() const noexcept -> int {
    return U32StringReadTools{dataView()}.displayWidth();
}

auto U32String::indexAt(const StringSide side) const noexcept -> CpIndex {
    return side == StringSide::Front ? CpIndex::zero() : CpIndex::end(length());
}

auto U32String::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(CpIndex::zero());
    }
    auto index = indexAt(StringSide::Back);
    if (!retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U32String::charAt(const CpIndex startIndex) const noexcept -> Char {
    return U32StringReadTools{dataView()}.charAt(startIndex);
}

auto U32String::readCharAndAdvance(CpIndex &index) const noexcept -> Char {
    return U32StringReadTools{dataView()}.read(index);
}

auto U32String::readCharAndRetreat(CpIndex &index) const noexcept -> Char {
    return U32StringReadTools{dataView()}.readAndRetreat(index);
}

auto U32String::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U32String::advance(CpIndex &index, const CpLength count) const noexcept -> bool {
    return U32StringReadTools{dataView()}.advance(index, count);
}

auto U32String::retreat(CpIndex &index, const CpLength count) const noexcept -> bool {
    return U32StringReadTools{dataView()}.retreat(index, count);
}

auto U32String::indexAt(const CpIndex index) const noexcept -> CpIndex {
    return index;
}

auto U32String::toCharIndex(const CpIndex index) const noexcept -> CpIndex {
    return index;
}

auto U32String::slice(const CpRange range) const noexcept -> U32String {
    return withRange(U32StringReadTools{dataView()}.sliceRange(range));
}

auto U32String::slice(const StringSide side, const CpLength length) const noexcept -> U32String {
    if (side == StringSide::Front) {
        return slice(CpRange(CpIndex::zero(), length));
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(CpRange{start, indexAt(StringSide::Back)});
}

auto U32String::slice(const StringSide side, const CpIndex index) const noexcept -> U32String {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(CpRange{CpIndex::zero(), end}) : U32String{};
    }
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), index});
    }
    return slice(CpRange{index, end});
}

auto U32String::slice(const StringSide side) const noexcept -> std::tuple<Char, U32String> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = CpIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(CpRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(CpRange{indexAt(StringSide::Front), start})};
}

auto U32String::splitAt(const CpIndex index) const noexcept -> std::pair<U32String, U32String> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U32String::removed(const CpRange range) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.removed(range)};
}

auto U32String::removedAll(const CharSet &characters) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.removedAll(characters)};
}

auto U32String::removedAll(const U32String &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U32String::removedFirst(const U32String &text, const CharCompareFn compareFn) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U32String::kept(const CpRange range) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.kept(range)};
}

auto U32String::inserted(const CpIndex index, const U32String &text) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U32String::replaced(const CpRange range, const U32String &text) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U32String::replacedAll(const CharSet &characters, const Char replacement) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U32String::replacedAll(const CharSet &characters, const U32String &replacement) const -> U32String {
    return U32String{U32StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U32String::replacedAll(const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const
    -> U32String {
    return U32String{U32StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32String::replacedFirst(const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const
    -> U32String {
    return U32String{
        U32StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32String::truncated(const CpLength maximumWidth, const TruncateMode mode) const -> U32String {
    return truncated(maximumWidth, mode, U32String{});
}

auto U32String::truncated(const CpLength maximumWidth, const TruncateMode mode, const U32String &ellipsis) const
    -> U32String {
    return U32String{U32StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U32String::aligned(const CpLength length, const geometry::Alignment alignment, const Char fill) const
    -> U32String {
    return U32String{U32StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U32String::toSafeString(const CpLength maximumWidth, const SafeStringFlags flags) const -> U32String {
    if (auto result =
            StringSafeTransformTools{*this, dataView().dataSpan(), maximumWidth, flags}.toSafeStringIfChanged()) {
        return U32String{result.value()};
    }
    return *this;
}

auto U32String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> CpLength {
    return U32StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U32String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32String {
    return U32StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U32String::fromCharacter(const Char character, const CpLength count) -> U32String {
    auto storage = U32StringSharedStorage{};
    U32StringAppendTools{storage}.append(character, count);
    return U32String{std::move(storage)};
}

auto U32String::fromJoined(const std::initializer_list<U32String> parts) -> U32String {
    auto finalSize = std::size_t{0};
    for (const auto &part : parts) {
        finalSize = U32StringSharedStorage::checkedAddSize(
            finalSize, part.length().toSizeT(), "Joined string exceeds size bounds");
    }
    if (finalSize == 0U) {
        return {};
    }

    auto storage = U32StringSharedStorage::forSize(finalSize);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size() * sizeof(char32_t));
        writePosition = U32StringSharedStorage::checkedAddSize(
            writePosition, data.size(), "Joined string write exceeds size bounds");
    }
    return U32String{std::move(storage)};
}

auto U32String::fromFloat(const double value, const FloatFormat format) -> U32String {
    return StringConverter{impl::formatFloat(value, format)}.toU32String();
}

auto U32String::fromBoolean(const bool value, const BooleanFormat format) -> U32String {
    return StringConverter{format.text(value)}.toU32String();
}

auto U32String::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U32String {
    auto storage = U32StringSharedStorage{};
    auto appendTools = U32StringAppendTools{storage};
    impl::formatByteBlock(appendTools, bytes, format);
    return U32String{std::move(storage)};
}

auto U32String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U32StringTransformTools{dataView()}.forEach(function);
}

auto U32String::transformed(const TransformCharacterFn function) const -> U32String {
    if (auto result = U32StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U32String{std::move(*result)};
    }
    return *this;
}

auto U32String::begin() const noexcept -> const_iterator {
    return U32StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U32String::end() const noexcept -> const_iterator {
    return U32StringConstIterator{*this, indexAt(StringSide::Back)};
}

void swap(U32String &first, U32String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U32String::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<U32StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<U32StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U32String::withRange(const CpRange range) const noexcept -> U32String {
    if (range.isEmpty()) {
        return {};
    }
    if (const auto *shared = std::get_if<U32StringSharedStorage>(&_storage)) {
        return U32String{U32StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<U32StringLiteralStorage>(&_storage)) {
        return U32String{U32StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U32String::dataView() const noexcept -> U32StringDataView {
    if (const auto *shared = std::get_if<U32StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<U32StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

auto U32String::isStorageShared() const noexcept -> bool {
    if (const auto *shared = std::get_if<U32StringSharedStorage>(&_storage)) {
        return shared->sharedData().isShared();
    }
    return false;
}

}
