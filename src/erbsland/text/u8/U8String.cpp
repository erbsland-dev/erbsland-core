// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "U8StringConstIterator.hpp"
#include "U8StringEditor.hpp"
#include "U8StringEditorList.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"

#include "impl/U8StringAppendTools.hpp"
#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringData.hpp"
#include "impl/U8StringEncodingTools.hpp"
#include "impl/U8StringReadTools.hpp"
#include "impl/U8StringTransformTools.hpp"

#include "../EncodingMode.hpp"
#include "../impl/ByteBlockFormatter.hpp"
#include "../impl/escape/StringSafeTransformTools.hpp"
#include "../impl/FloatConversion.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/U32StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>
#include <utility>

namespace erbsland::text {

using namespace impl;
using namespace unit;

U8String::U8String(const std::string_view stdString) : _storage{U8StringSharedStorage{stdString}} {
}

U8String::U8String(const std::u8string_view stdString) : _storage{U8StringSharedStorage{stdString}} {
}

U8String::U8String(const U8StringEditor &str) noexcept : _storage{str._storage} {
}

U8String::U8String(const U8StringLiteral<char> &str) noexcept :
    _storage{U8StringLiteralStorage{str._charPtr, str._size}} {
}

U8String::U8String(const U8StringLiteral<char8_t> &str) noexcept :
    _storage{U8StringLiteralStorage{str._charPtr, str._size}} {
}

auto U8String::copy() const -> U8String {
    return U8String{U8StringSharedStorage{dataView(), isSensitive()}};
}

auto U8String::isEmpty() const noexcept -> bool {
    return U8StringReadTools{dataView()}.isEmpty();
}

auto U8String::isSensitive() const noexcept -> bool {
    const auto *shared = std::get_if<U8StringSharedStorage>(&_storage);
    return shared != nullptr && shared->isSensitive();
}

void U8String::markAsSensitive() noexcept {
    if (auto *shared = std::get_if<U8StringSharedStorage>(&_storage)) {
        shared->markAsSensitive();
        return;
    }
    if (const auto *literal = std::get_if<U8StringLiteralStorage>(&_storage)) {
        _storage = U8StringSharedStorage{*literal, true};
        return;
    }
    _storage = U8StringSharedStorage::forSize(0U, true);
}

auto U8String::isEncodingValid() const noexcept -> bool {
    return U8StringReadTools{dataView()}.isEncodingValid();
}

void U8String::encodingValidOrThrow() const {
    U8StringReadTools{dataView()}.encodingValidOrThrow();
}

auto U8String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U8String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf8::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U8String::findFirstOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstOf(characters);
}

auto U8String::findFirstOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U8String::findFirstNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U8String::findFirstNotOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U8String::findLastOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastOf(characters);
}

auto U8String::findLastOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U8String::findLastNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U8String::findLastNotOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U8String::find(const U8String &text, const CharCompareFn compareFn) const noexcept -> ByteIndex {
    return U8StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U8String::find(const U8String &text, const ByteIndex start, const CharCompareFn compareFn) const noexcept
    -> ByteIndex {
    return U8StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U8String::length() const noexcept -> ByteCount {
    return U8StringReadTools{dataView()}.byteLength();
}

auto U8String::characterLength() const noexcept -> CpCount {
    return U8StringCharReadTool{dataView()}.charLength();
}

auto U8String::displayWidth() const noexcept -> int {
    return U8StringReadTools{dataView()}.displayWidth();
}

auto U8String::indexAt(const StringSide side) const noexcept -> ByteIndex {
    return side == StringSide::Front ? ByteIndex::zero() : ByteIndex::end(length());
}

auto U8String::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(ByteIndex::zero());
    }
    auto index = ByteIndex::end(length());
    if (!U8StringReadTools{dataView()}.retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U8String::charAt(const ByteIndex startIndex) const noexcept -> Char {
    return U8StringReadTools{dataView()}.charAt(startIndex);
}

auto U8String::readCharAndAdvance(ByteIndex &index) const noexcept -> Char {
    return U8StringReadTools{dataView()}.read(index);
}

auto U8String::readCharAndRetreat(ByteIndex &index) const noexcept -> Char {
    return U8StringReadTools{dataView()}.readAndRetreat(index);
}

auto U8String::charAt(const CpIndex index) const noexcept -> Char {
    return U8StringCharReadTool{dataView()}.charAt(index);
}

auto U8String::operator[](const ByteIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8String::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8String::advance(ByteIndex &index, const CpCount count) const noexcept -> bool {
    return U8StringReadTools{dataView()}.advance(index, count);
}

auto U8String::retreat(ByteIndex &index, const CpCount count) const noexcept -> bool {
    return U8StringReadTools{dataView()}.retreat(index, count);
}

auto U8String::indexAt(const CpIndex index) const noexcept -> ByteIndex {
    return U8StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U8String::toCharIndex(const ByteIndex index) const noexcept -> CpIndex {
    return U8StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U8String::slice(const ByteRange range) const noexcept -> U8String {
    return withRange(U8StringReadTools{dataView()}.sliceRange(range));
}

auto U8String::slice(const CpRange range) const noexcept -> U8String {
    return withRange(U8StringCharReadTool{dataView()}.sliceRange(range));
}

auto U8String::slice(const StringSide side, const ByteCount length) const noexcept -> U8String {
    if (side == StringSide::Front) {
        return slice(ByteRange(ByteIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(ByteRange::fromLength(fullLength));
    }
    const auto start = ByteIndex::end(fullLength - length);
    return slice(ByteRange{start, length});
}

auto U8String::slice(const StringSide side, const ByteIndex index) const noexcept -> U8String {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(ByteRange{ByteIndex::zero(), end}) : U8String{};
    }
    if (side == StringSide::Front) {
        return slice(ByteRange{ByteIndex::zero(), index});
    }
    return slice(ByteRange{index, end});
}

auto U8String::slice(const StringSide side, const CpCount length) const noexcept -> U8String {
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(ByteRange{start, indexAt(StringSide::Back)});
}

auto U8String::slice(const StringSide side, const CpIndex index) const noexcept -> U8String {
    return slice(side, indexAt(index));
}

auto U8String::slice(const StringSide side) const noexcept -> std::tuple<Char, U8String> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = ByteIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(ByteRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(ByteRange{indexAt(StringSide::Front), start})};
}

auto U8String::splitAt(const ByteIndex index) const noexcept -> std::pair<U8String, U8String> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U8String::splitAt(const CpIndex index) const noexcept -> std::pair<U8String, U8String> {
    return splitAt(indexAt(index));
}

auto U8String::truncated(const CpCount maximumWidth, const TruncateMode mode) const -> U8String {
    return truncated(maximumWidth, mode, U8String{});
}

auto U8String::truncated(const CpCount maximumWidth, const TruncateMode mode, const U8String &ellipsis) const
    -> U8String {
    if (auto result = U8StringTransformTools{dataView(), isSensitive()}.truncatedIfChanged(
            maximumWidth, mode, ellipsis.dataView())) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::aligned(const CpCount length, const geometry::Alignment alignment, const Char fill) const -> U8String {
    if (auto result = U8StringTransformTools{dataView(), isSensitive()}.alignedIfChanged(length, alignment, fill)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::toSafeString(const CpCount maximumWidth, const SafeStringFlags flags) const -> U8String {
    if (auto result =
            StringSafeTransformTools{*this, dataView().dataSpan(), maximumWidth, flags}.toSafeStringIfChanged()) {
        return U8String{result.value()};
    }
    return *this;
}

auto U8String::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> ByteCount {
    return U8StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U8String::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U8String {
    if (auto result = U8StringTransformTools{dataView(), isSensitive()}.toEscapedIfChanged(*this, format, amount)) {
        return U8String{std::move(result.value())};
    }
    return *this;
}

auto U8String::fromCharacter(const Char character, const CpCount count) -> U8String {
    auto storage = U8StringSharedStorage{};
    U8StringAppendTools{storage}.append(character, count);
    return U8String{std::move(storage)};
}

auto U8String::fromJoined(const std::initializer_list<U8String> parts) -> U8String {
    auto finalSize = std::size_t{0};
    auto sensitive = false;
    for (const auto &part : parts) {
        sensitive = sensitive || part.isSensitive();
        finalSize = U8StringSharedStorage::checkedAddSize(finalSize, part.length().toSizeT());
    }
    if (finalSize == 0U && !sensitive) {
        return {};
    }

    auto storage = U8StringSharedStorage::forSize(finalSize, sensitive);
    auto writePosition = std::size_t{0};
    for (const auto &part : parts) {
        const auto data = part.dataView().dataSpan();
        if (data.empty()) {
            continue;
        }
        std::memcpy(storage.dataForWrite() + writePosition, data.data(), data.size());
        writePosition = U8StringSharedStorage::checkedAddSize(writePosition, data.size());
    }
    return U8String{std::move(storage)};
}

auto U8String::fromFloat(const double value, const FloatFormat format) -> U8String {
    return impl::formatFloat(value, format);
}

auto U8String::fromBoolean(const bool value, const BooleanFormat format) -> U8String {
    return U8String{format.text(value)};
}

auto U8String::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U8String {
    auto storage = U8StringSharedStorage{};
    auto appendTools = U8StringAppendTools{storage};
    impl::formatByteBlock(appendTools, bytes, format);
    return U8String{std::move(storage)};
}

auto U8String::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U8StringTransformTools{dataView()}.forEach(function);
}

auto U8String::forEach(const ProcessCharacterWithCpIndexFn &function) const -> util::LoopResult {
    return U8StringTransformTools{dataView()}.forEach(function);
}

auto U8String::transformed(const TransformCharacterFn function) const -> U8String {
    if (auto result = U8StringTransformTools{dataView(), isSensitive()}.transformedIfChanged(function)) {
        return U8String{std::move(*result)};
    }
    return *this;
}

auto U8String::begin() const noexcept -> const_iterator {
    return U8StringConstIterator{*this, indexAt(StringSide::Front)};
}

auto U8String::end() const noexcept -> const_iterator {
    return U8StringConstIterator{*this, indexAt(StringSide::Back)};
}

void swap(U8String &first, U8String &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U8String::storageId() const noexcept -> mem::StorageIdentifier {
    if (const auto *shared = std::get_if<U8StringSharedStorage>(&_storage)) {
        return shared->storageId();
    }
    if (const auto *literal = std::get_if<U8StringLiteralStorage>(&_storage)) {
        return literal->storageId();
    }
    return {};
}

auto U8String::withRange(const ByteRange range) const noexcept -> U8String {
    if (range.isEmpty()) {
        return U8String{U8StringSharedStorage::forSize(0U, isSensitive())};
    }
    if (const auto *shared = std::get_if<U8StringSharedStorage>(&_storage)) {
        return U8String{U8StringSharedStorage{shared->sharedData(), range}};
    }
    if (const auto *literal = std::get_if<U8StringLiteralStorage>(&_storage)) {
        return U8String{U8StringLiteralStorage{literal->data(), range}};
    }
    return {};
}

auto U8String::dataView() const noexcept -> U8StringDataView {
    if (const auto *shared = std::get_if<U8StringSharedStorage>(&_storage)) {
        return shared->dataView();
    }
    if (const auto *literal = std::get_if<U8StringLiteralStorage>(&_storage)) {
        return literal->dataView();
    }
    return {};
}

auto U8String::isStorageShared() const noexcept -> bool {
    if (const auto *shared = std::get_if<U8StringSharedStorage>(&_storage)) {
        return shared->sharedData().isShared();
    }
    return false;
}

}
