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

#include <cstring>

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

auto U8String::displayWidth() const noexcept -> int {
    return impl::U8StringReadTools{dataView()}.displayWidth();
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

auto U8String::readCharAndAdvance(unit::ByteIndex &index) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.read(index);
}

auto U8String::readCharAndRetreat(unit::ByteIndex &index) const noexcept -> Char {
    return impl::U8StringReadTools{dataView()}.readAndRetreat(index);
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
    const auto start = unit::ByteIndex::end(fullLength - length);
    return slice(unit::ByteRange{start, length});
}

auto U8String::slice(const StringSide side, const unit::ByteIndex index) const noexcept -> U8String {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(unit::ByteRange{unit::ByteIndex::zero(), end}) : U8String{};
    }
    if (side == StringSide::Front) {
        return slice(unit::ByteRange{unit::ByteIndex::zero(), index});
    }
    return slice(unit::ByteRange{index, end});
}

auto U8String::slice(const StringSide side, const unit::CpLength length) const noexcept -> U8String {
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::ByteRange{start, indexAt(StringSide::Back)});
}

auto U8String::slice(const StringSide side, const unit::CpIndex index) const noexcept -> U8String {
    return slice(side, indexAt(index));
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

auto U8String::splitAt(const unit::ByteIndex index) const noexcept -> std::pair<U8String, U8String> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U8String::splitAt(const unit::CpIndex index) const noexcept -> std::pair<U8String, U8String> {
    return splitAt(indexAt(index));
}

}
