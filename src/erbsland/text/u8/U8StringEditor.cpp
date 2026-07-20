// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "U8String.hpp"
#include "U8StringEditorList.hpp"
#include "U8StringList.hpp"
#include "U8StringLiteral.hpp"

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
#include "../u16/U16StringEditor.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

using impl::U8StringCharReadTool;
using impl::U8StringComparisonTools;
using impl::U8StringReadTools;
using namespace unit;

U8StringEditor::U8StringEditor(const std::string_view stdString) : _storage{stdString} {
}

U8StringEditor::U8StringEditor(const std::u8string_view stdString) : _storage{stdString} {
}

U8StringEditor::U8StringEditor(const U8StringLiteral<char> &literal) : _storage{literal.dataView()} {
}

U8StringEditor::U8StringEditor(const U8StringLiteral<char8_t> &literal) : _storage{literal.dataView()} {
}

U8StringEditor::U8StringEditor(const U8String &view) : _storage{view.dataView()} {
}

auto U8StringEditor::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U8StringEditor::isValidUtf8() const noexcept -> bool {
    return U8StringReadTools{dataView()}.isValidUtf8();
}

auto U8StringEditor::findFirstOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstOf(characters);
}

auto U8StringEditor::findFirstOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U8StringEditor::findFirstNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U8StringEditor::findFirstNotOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U8StringEditor::findLastOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastOf(characters);
}

auto U8StringEditor::findLastOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U8StringEditor::findLastNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U8StringEditor::findLastNotOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return U8StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U8StringEditor::find(const U8String &text, const CharCompareFn compareFn) const noexcept -> ByteIndex {
    return U8StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U8StringEditor::find(const U8String &text, const ByteIndex start, const CharCompareFn compareFn) const noexcept
    -> ByteIndex {
    return U8StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U8StringEditor::length() const noexcept -> ByteLength {
    return U8StringReadTools{dataView()}.byteLength();
}

auto U8StringEditor::characterLength() const noexcept -> CpLength {
    return U8StringCharReadTool{dataView()}.charLength();
}

auto U8StringEditor::displayWidth() const noexcept -> int {
    return U8StringReadTools{dataView()}.displayWidth();
}

auto U8StringEditor::indexAt(const StringSide side) const noexcept -> ByteIndex {
    return side == StringSide::Front ? ByteIndex::zero() : ByteIndex::end(length());
}

auto U8StringEditor::charAt(const StringSide side) const noexcept -> Char {
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

auto U8StringEditor::charAt(const ByteIndex startIndex) const noexcept -> Char {
    return U8StringReadTools{dataView()}.charAt(startIndex);
}

auto U8StringEditor::readCharAndAdvance(ByteIndex &index) const noexcept -> Char {
    return U8StringReadTools{dataView()}.read(index);
}

auto U8StringEditor::readCharAndAdvanceOrThrow(ByteIndex &index) const -> Char {
    return U8StringReadTools{dataView()}.readOrThrow(index);
}

auto U8StringEditor::readCharAndRetreat(ByteIndex &index) const noexcept -> Char {
    return U8StringReadTools{dataView()}.readAndRetreat(index);
}

auto U8StringEditor::charAt(const CpIndex index) const noexcept -> Char {
    return U8StringCharReadTool{dataView()}.charAt(index);
}

auto U8StringEditor::operator[](const ByteIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8StringEditor::operator[](const CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U8StringEditor::advance(ByteIndex &index, const CpLength count) const noexcept -> bool {
    return U8StringReadTools{dataView()}.advance(index, count);
}

auto U8StringEditor::retreat(ByteIndex &index, const CpLength count) const noexcept -> bool {
    return U8StringReadTools{dataView()}.retreat(index, count);
}

auto U8StringEditor::indexAt(const CpIndex index) const noexcept -> ByteIndex {
    return U8StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U8StringEditor::toCharIndex(const ByteIndex index) const noexcept -> CpIndex {
    return U8StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U8StringEditor::slice(const ByteRange range) const noexcept -> U8StringEditor {
    return withRange(U8StringReadTools{dataView()}.sliceRange(range));
}

auto U8StringEditor::slice(const CpRange range) const noexcept -> U8StringEditor {
    return withRange(U8StringCharReadTool{dataView()}.sliceRange(range));
}

auto U8StringEditor::slice(const StringSide side, const ByteLength length) const noexcept -> U8StringEditor {
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

auto U8StringEditor::slice(const StringSide side, const ByteIndex index) const noexcept -> U8StringEditor {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(ByteRange{ByteIndex::zero(), end}) : U8StringEditor{};
    }
    if (side == StringSide::Front) {
        return slice(ByteRange{ByteIndex::zero(), index});
    }
    return slice(ByteRange{index, end});
}

auto U8StringEditor::slice(const StringSide side, const CpLength length) const noexcept -> U8StringEditor {
    if (side == StringSide::Front) {
        return slice(CpRange{CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(ByteRange{start, indexAt(StringSide::Back)});
}

auto U8StringEditor::slice(const StringSide side, const CpIndex index) const noexcept -> U8StringEditor {
    return slice(side, indexAt(index));
}

auto U8StringEditor::slice(const StringSide side) const noexcept -> std::tuple<Char, U8StringEditor> {
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

auto U8StringEditor::splitAt(const ByteIndex index) const noexcept -> std::pair<U8StringEditor, U8StringEditor> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U8StringEditor::splitAt(const CpIndex index) const noexcept -> std::pair<U8StringEditor, U8StringEditor> {
    return splitAt(indexAt(index));
}

}
