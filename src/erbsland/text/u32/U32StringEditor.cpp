// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEditor.hpp"

#include "U32String.hpp"
#include "U32StringConstIterator.hpp"
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
#include "impl/U32Writer.hpp"

#include "../EncodingMode.hpp"
#include "../StringConverter.hpp"
#include "../u16/impl/U16Encoding.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using namespace impl;
using namespace unit;

U32StringEditor::U32StringEditor(const std::u32string_view stdString) : _storage{stdString} {
}

U32StringEditor::U32StringEditor(const U32StringLiteral &literal) : _storage{literal.dataView()} {
}

U32StringEditor::U32StringEditor(const U32String &view) : _storage{view.dataView()} {
}

auto U32StringEditor::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U32StringEditor::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U32StringEditor::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf32::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U32StringEditor::isValidUtf32() const noexcept -> bool {
    return U32StringReadTools{dataView()}.isValidUtf32();
}

auto U32StringEditor::findFirstOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstOf(characters);
}

auto U32StringEditor::findFirstOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U32StringEditor::findFirstNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U32StringEditor::findFirstNotOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U32StringEditor::findLastOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastOf(characters);
}

auto U32StringEditor::findLastOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U32StringEditor::findLastNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U32StringEditor::findLastNotOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return U32StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U32StringEditor::find(const U32String &text, const CharCompareFn compareFn) const noexcept -> CpIndex {
    return U32StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U32StringEditor::find(const U32String &text, const CpIndex start, const CharCompareFn compareFn) const noexcept
    -> CpIndex {
    return U32StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U32StringEditor::length() const noexcept -> CpLength {
    return U32StringReadTools{dataView()}.length();
}

auto U32StringEditor::characterLength() const noexcept -> CpLength {
    return U32StringReadTools{dataView()}.length();
}

auto U32StringEditor::displayWidth() const noexcept -> int {
    return U32StringReadTools{dataView()}.displayWidth();
}

auto U32StringEditor::indexAt(const StringSide side) const noexcept -> CpIndex {
    return side == StringSide::Front ? CpIndex::zero() : CpIndex::end(length());
}

auto U32StringEditor::charAt(const StringSide side) const noexcept -> Char {
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

auto U32StringEditor::charAt(const CpIndex startIndex) const noexcept -> Char {
    return U32StringReadTools{dataView()}.charAt(startIndex);
}

auto U32StringEditor::readCharAndAdvance(CpIndex &index) const noexcept -> Char {
    return U32StringReadTools{dataView()}.read(index);
}

auto U32StringEditor::readCharAndRetreat(CpIndex &index) const noexcept -> Char {
    return U32StringReadTools{dataView()}.readAndRetreat(index);
}

auto U32StringEditor::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U32StringEditor::advance(CpIndex &index, const CpLength count) const noexcept -> bool {
    return U32StringReadTools{dataView()}.advance(index, count);
}

auto U32StringEditor::retreat(CpIndex &index, const CpLength count) const noexcept -> bool {
    return U32StringReadTools{dataView()}.retreat(index, count);
}

auto U32StringEditor::indexAt(const CpIndex index) const noexcept -> CpIndex {
    return index;
}

auto U32StringEditor::toCharIndex(const CpIndex index) const noexcept -> CpIndex {
    return index;
}

auto U32StringEditor::slice(const CpRange range) const noexcept -> U32StringEditor {
    return withRange(U32StringReadTools{dataView()}.sliceRange(range));
}

auto U32StringEditor::slice(const StringSide side, const CpLength length) const noexcept -> U32StringEditor {
    if (side == StringSide::Front) {
        return slice(CpRange(CpIndex::zero(), length));
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(CpRange{start, indexAt(StringSide::Back)});
}

auto U32StringEditor::slice(const StringSide side, const CpIndex index) const noexcept -> U32StringEditor {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(CpRange{CpIndex::zero(), end}) : U32StringEditor{};
    }
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), index});
    }
    return slice(CpRange{index, end});
}

auto U32StringEditor::slice(const StringSide side) const noexcept -> std::tuple<Char, U32StringEditor> {
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

auto U32StringEditor::splitAt(const CpIndex index) const noexcept -> std::pair<U32StringEditor, U32StringEditor> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U32StringEditor::clear() noexcept -> U32StringEditor & {
    _storage.clear();
    return *this;
}

auto U32StringEditor::append(const U32String &text, const ItemCount count) -> U32StringEditor & {
    U32StringAppendTools{_storage}.append(text.dataView(), count);
    return *this;
}

auto U32StringEditor::append(const Char character, CpLength count) -> U32StringEditor & {
    U32StringAppendTools{_storage}.append(character, count);
    return *this;
}

auto U32StringEditor::remove(const CpRange range) -> U32StringEditor & {
    U32StringModifyTools::remove(_storage, range);
    return *this;
}

auto U32StringEditor::removeAll(const CharSet &characters) -> U32StringEditor & {
    U32StringModifyTools::removeAll(_storage, characters);
    return *this;
}

auto U32StringEditor::removeAll(const U32String &text, const CharCompareFn compareFn) -> U32StringEditor & {
    U32StringModifyTools::remove(_storage, text.dataView(), compareFn);
    return *this;
}

auto U32StringEditor::removeFirst(const U32String &text, const CharCompareFn compareFn) -> U32StringEditor & {
    U32StringModifyTools::removeFirst(_storage, text.dataView(), compareFn);
    return *this;
}

auto U32StringEditor::keep(const CpRange range) -> U32StringEditor & {
    U32StringModifyTools::keep(_storage, range);
    return *this;
}

auto U32StringEditor::insert(const CpIndex index, const U32String &text) -> U32StringEditor & {
    U32StringModifyTools::insert(_storage, index, text.dataView());
    return *this;
}

auto U32StringEditor::replace(const CpRange range, const U32String &text) -> U32StringEditor & {
    U32StringModifyTools::replace(_storage, range, text.dataView());
    return *this;
}

auto U32StringEditor::replaceFirst(const U32String &text, const U32String &replacement, const CharCompareFn compareFn)
    -> U32StringEditor & {
    U32StringModifyTools::replaceFirst(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U32StringEditor::replaceAll(const CharSet &characters, const Char replacement) -> U32StringEditor & {
    U32StringModifyTools::replaceAll(_storage, characters, replacement);
    return *this;
}

auto U32StringEditor::replaceAll(const CharSet &characters, const U32String &replacement) -> U32StringEditor & {
    U32StringModifyTools::replaceAll(_storage, characters, replacement.dataView());
    return *this;
}

auto U32StringEditor::replaceAll(const U32String &text, const U32String &replacement, const CharCompareFn compareFn)
    -> U32StringEditor & {
    U32StringModifyTools::replaceAll(_storage, text.dataView(), replacement.dataView(), compareFn);
    return *this;
}

auto U32StringEditor::truncate(const CpLength maximumWidth, const TruncateMode mode) -> U32StringEditor & {
    return truncate(maximumWidth, mode, U32String{});
}

auto U32StringEditor::truncate(const CpLength maximumWidth, const TruncateMode mode, const U32String &ellipsis)
    -> U32StringEditor & {
    *this = truncated(maximumWidth, mode, ellipsis);
    return *this;
}

auto U32StringEditor::removed(const CpRange range) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.removed(range)};
}

auto U32StringEditor::removedAll(const CharSet &characters) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.removedAll(characters)};
}

auto U32StringEditor::removedAll(const U32String &text, const CharCompareFn compareFn) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.removed(text.dataView(), compareFn)};
}

auto U32StringEditor::removedFirst(const U32String &text, const CharCompareFn compareFn) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.removedFirst(text.dataView(), compareFn)};
}

auto U32StringEditor::kept(const CpRange range) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.kept(range)};
}

auto U32StringEditor::inserted(const CpIndex index, const U32String &text) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.inserted(index, text.dataView())};
}

auto U32StringEditor::replaced(const CpRange range, const U32String &text) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.replaced(range, text.dataView())};
}

auto U32StringEditor::replacedFirst(
    const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const -> U32StringEditor {
    return U32StringEditor{
        U32StringModifyTools{dataView()}.replacedFirst(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32StringEditor::replacedAll(const CharSet &characters, const Char replacement) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.replacedAll(characters, replacement)};
}

auto U32StringEditor::replacedAll(const CharSet &characters, const U32String &replacement) const -> U32StringEditor {
    return U32StringEditor{U32StringModifyTools{dataView()}.replacedAll(characters, replacement.dataView())};
}

auto U32StringEditor::replacedAll(
    const U32String &text, const U32String &replacement, const CharCompareFn compareFn) const -> U32StringEditor {
    return U32StringEditor{
        U32StringModifyTools{dataView()}.replacedAll(text.dataView(), replacement.dataView(), compareFn)};
}

auto U32StringEditor::truncated(const CpLength maximumWidth, const TruncateMode mode) const -> U32StringEditor {
    return truncated(maximumWidth, mode, U32String{});
}

auto U32StringEditor::truncated(const CpLength maximumWidth, const TruncateMode mode, const U32String &ellipsis) const
    -> U32StringEditor {
    return U32StringEditor{U32StringTransformTools{dataView()}.truncated(maximumWidth, mode, ellipsis.dataView())};
}

auto U32StringEditor::aligned(const CpLength length, const bgeo::Alignment alignment, const Char fill) const
    -> U32StringEditor {
    return U32StringEditor{U32StringTransformTools{dataView()}.aligned(length, alignment, fill)};
}

auto U32StringEditor::toSafeString(const CpLength maximumWidth, const SafeStringFlags flags) const -> U32StringEditor {
    return U32StringTransformTools{dataView()}.toSafeString(maximumWidth, flags);
}

void swap(U32StringEditor &first, U32StringEditor &second) noexcept {
    using std::swap;
    swap(first._storage, second._storage);
}

auto U32StringEditor::escapedSize(const EscapeFormat format, const EscapeAmount amount) const noexcept -> CpLength {
    return U32StringTransformTools{dataView()}.escapedSize(format, amount);
}

auto U32StringEditor::toEscaped(const EscapeFormat format, const EscapeAmount amount) const -> U32StringEditor {
    return U32StringTransformTools{dataView()}.toEscaped(format, amount);
}

auto U32StringEditor::fromCharacter(const Char character, const CpLength count) -> U32StringEditor {
    auto storage = U32StringSharedStorage{};
    U32StringAppendTools{storage}.append(character, count);
    return U32StringEditor{std::move(storage)};
}

auto U32StringEditor::fromJoined(const std::initializer_list<U32String> parts) -> U32StringEditor {
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
    return U32StringEditor{std::move(storage)};
}

auto U32StringEditor::fromBoolean(const bool value, const BooleanFormat format) -> U32StringEditor {
    return U32StringEditor{StringConverter{format.text(value)}.toU32String()};
}

auto U32StringEditor::fromByteBlock(const mem::ByteBlock &bytes, const ByteFormat format) -> U32StringEditor {
    auto builder = AnyStringBuilder{StringKind::U32};
    builder.appendByteBlock(bytes, format);
    return builder.takeU32StringEditor();
}

auto U32StringEditor::forEach(const ProcessCharacterFn &function) const -> util::LoopResult {
    return U32StringTransformTools{dataView()}.forEach(function);
}

auto U32StringEditor::transformed(const TransformCharacterFn function) const -> U32StringEditor {
    if (auto result = U32StringTransformTools{dataView()}.transformedIfChanged(function)) {
        return U32StringEditor{std::move(*result)};
    }
    return *this;
}

void U32StringEditor::reset() noexcept {
    _storage = {};
}

auto U32StringEditor::storageId() const noexcept -> mem::StorageIdentifier {
    return _storage.storageId();
}

void U32StringEditor::reserve(const CpLength capacity) {
    _storage.reserve(capacity);
}

void U32StringEditor::shrinkToFit() {
    _storage.shrinkToFit();
}

auto U32StringEditor::capacity() const noexcept -> CpLength {
    return _storage.capacity();
}

auto U32StringEditor::memoryUsage() const noexcept -> ByteLength {
    return _storage.memoryUsage();
}

void U32StringEditor::detach() {
    _storage.detach();
}

auto U32StringEditor::begin() const noexcept -> const_iterator {
    return U32StringConstIterator{U32String{*this}, indexAt(StringSide::Front)};
}

auto U32StringEditor::end() const noexcept -> const_iterator {
    return U32StringConstIterator{U32String{*this}, indexAt(StringSide::Back)};
}

auto U32StringEditor::withRange(const CpRange range) const noexcept -> U32StringEditor {
    if (range.isEmpty()) {
        return {};
    }
    return U32StringEditor{U32StringSharedStorage{_storage.sharedData(), range}};
}

auto U32StringEditor::dataView() const noexcept -> U32StringDataView {
    return _storage.dataView();
}

auto U32StringEditor::isStorageShared() const noexcept -> bool {
    return _storage.sharedData().isShared();
}

}
