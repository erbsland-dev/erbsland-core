// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "U16String.hpp"
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
#include "impl/U16Writer.hpp"

#include "../EncodingMode.hpp"
#include "../StringConverter.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using impl::U16StringCharReadTool;
using impl::U16StringComparisonTools;
using impl::U16StringReadTools;
using namespace unit;

U16StringEditor::U16StringEditor(const std::u16string_view stdString) : _storage{stdString} {
}

U16StringEditor::U16StringEditor(const U16StringLiteral &literal) : _storage{literal.dataView()} {
}

U16StringEditor::U16StringEditor(const U16String &view) : _storage{view.dataView()} {
}

auto U16StringEditor::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U16StringEditor::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.toRawValue());
            return true;
        });
    return result;
}

auto U16StringEditor::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(
        dataView().dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
            util::advanceHash(result, character.caseFolded().toRawValue());
            return true;
        });
    return result;
}

auto U16StringEditor::isValidUtf16() const noexcept -> bool {
    return U16StringReadTools{dataView()}.isValidUtf16();
}

auto U16StringEditor::findFirstOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstOf(characters);
}

auto U16StringEditor::findFirstOf(const CharSet &characters, const U16DataIndex start) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U16StringEditor::findFirstNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U16StringEditor::findFirstNotOf(const CharSet &characters, const U16DataIndex start) const noexcept
    -> U16DataIndex {
    return U16StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U16StringEditor::findLastOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastOf(characters);
}

auto U16StringEditor::findLastOf(const CharSet &characters, const U16DataIndex end) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U16StringEditor::findLastNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U16StringEditor::findLastNotOf(const CharSet &characters, const U16DataIndex end) const noexcept -> U16DataIndex {
    return U16StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U16StringEditor::find(const U16String &text, const CharCompareFn compareFn) const noexcept -> U16DataIndex {
    return U16StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U16StringEditor::find(
    const U16String &text, const U16DataIndex start, const CharCompareFn compareFn) const noexcept -> U16DataIndex {
    return U16StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U16StringEditor::length() const noexcept -> U16DataLength {
    return U16StringReadTools{dataView()}.byteLength();
}

auto U16StringEditor::characterLength() const noexcept -> CpLength {
    return U16StringCharReadTool{dataView()}.charLength();
}

auto U16StringEditor::displayWidth() const noexcept -> int {
    return U16StringReadTools{dataView()}.displayWidth();
}

auto U16StringEditor::indexAt(const StringSide side) const noexcept -> U16DataIndex {
    return side == StringSide::Front ? U16DataIndex::zero() : U16DataIndex::end(length());
}

auto U16StringEditor::charAt(const StringSide side) const noexcept -> Char {
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

auto U16StringEditor::charAt(const U16DataIndex startIndex) const noexcept -> Char {
    return U16StringReadTools{dataView()}.charAt(startIndex);
}

auto U16StringEditor::readCharAndAdvance(U16DataIndex &index) const noexcept -> Char {
    return U16StringReadTools{dataView()}.read(index);
}

auto U16StringEditor::readCharAndRetreat(U16DataIndex &index) const noexcept -> Char {
    return U16StringReadTools{dataView()}.readAndRetreat(index);
}

auto U16StringEditor::charAt(const CpIndex index) const noexcept -> Char {
    return U16StringCharReadTool{dataView()}.charAt(index);
}

auto U16StringEditor::operator[](const U16DataIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16StringEditor::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16StringEditor::advance(U16DataIndex &index, const CpLength count) const noexcept -> bool {
    return U16StringReadTools{dataView()}.advance(index, count);
}

auto U16StringEditor::retreat(U16DataIndex &index, const CpLength count) const noexcept -> bool {
    return U16StringReadTools{dataView()}.retreat(index, count);
}

auto U16StringEditor::indexAt(const CpIndex index) const noexcept -> U16DataIndex {
    return U16StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U16StringEditor::toCharIndex(const U16DataIndex index) const noexcept -> CpIndex {
    return U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16StringEditor::slice(const U16DataRange range) const noexcept -> U16StringEditor {
    return withRange(U16StringReadTools{dataView()}.sliceRange(range));
}

auto U16StringEditor::slice(const CpRange range) const noexcept -> U16StringEditor {
    return withRange(U16StringCharReadTool{dataView()}.sliceRange(range));
}

auto U16StringEditor::slice(const StringSide side, const U16DataLength length) const noexcept -> U16StringEditor {
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

auto U16StringEditor::slice(const StringSide side, const U16DataIndex index) const noexcept -> U16StringEditor {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(U16DataRange{U16DataIndex::zero(), end}) : U16StringEditor{};
    }
    if (side == StringSide::Front) {
        return slice(U16DataRange{U16DataIndex::zero(), index});
    }
    return slice(U16DataRange{index, end});
}

auto U16StringEditor::slice(const StringSide side, const CpLength length) const noexcept -> U16StringEditor {
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(U16DataRange{start, indexAt(StringSide::Back)});
}

auto U16StringEditor::slice(const StringSide side, const CpIndex index) const noexcept -> U16StringEditor {
    return slice(side, indexAt(index));
}

auto U16StringEditor::slice(const StringSide side) const noexcept -> std::tuple<Char, U16StringEditor> {
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

auto U16StringEditor::splitAt(const U16DataIndex index) const noexcept -> std::pair<U16StringEditor, U16StringEditor> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U16StringEditor::splitAt(const CpIndex index) const noexcept -> std::pair<U16StringEditor, U16StringEditor> {
    return splitAt(indexAt(index));
}

}
