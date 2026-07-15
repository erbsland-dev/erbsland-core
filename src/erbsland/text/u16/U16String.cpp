// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "U16StringCharView.hpp"
#include "U16StringList.hpp"
#include "U16StringLiteral.hpp"
#include "U16StringView.hpp"
#include "U16StringViewList.hpp"

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

#include "../StringConverter.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/U32String.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/U8String.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../util/HashHelper.hpp"

#include <cstring>

namespace erbsland::text {

U16String::U16String(const std::u16string_view stdString) : _storage{stdString} {
}

U16String::U16String(const U16StringLiteral &literal) : _storage{literal.dataView()} {
}

U16String::U16String(const U16StringView &view) : _storage{view.dataView()} {
}

auto U16String::isEmpty() const noexcept -> bool {
    return _storage.isEmpty();
}

auto U16String::toHash() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.toRawValue());
        return true;
    });
    return result;
}

auto U16String::toHashCI() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    impl::utf16::forEachDecodedCharacter(dataView().dataSpan(), EncodingErrorMode::Replace, [&](const Char character) {
        util::advanceHash(result, character.caseFolded().toRawValue());
        return true;
    });
    return result;
}

auto U16String::isValidUtf16() const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.isValidUtf16();
}

auto U16String::findFirstOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstOf(characters);
}

auto U16String::findFirstOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstOf(characters, start);
}

auto U16String::findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstNotOf(characters);
}

auto U16String::findFirstNotOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findFirstNotOf(characters, start);
}

auto U16String::findLastOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastOf(characters);
}

auto U16String::findLastOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastOf(characters, end);
}

auto U16String::findLastNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastNotOf(characters);
}

auto U16String::findLastNotOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringReadTools{dataView()}.findLastNotOf(characters, end);
}

auto U16String::find(const U16StringView &text, const CharCompareFn compareFn) const noexcept -> unit::U16DataIndex {
    return impl::U16StringComparisonTools{dataView()}.find(text.dataView(), compareFn);
}

auto U16String::find(
    const U16StringView &text, const unit::U16DataIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::U16DataIndex {
    return impl::U16StringComparisonTools{dataView()}.find(text.dataView(), start, compareFn);
}

auto U16String::length() const noexcept -> unit::U16DataLength {
    return impl::U16StringReadTools{dataView()}.byteLength();
}

auto U16String::characterLength() const noexcept -> unit::CpLength {
    return impl::U16StringCharReadTool{dataView()}.charLength();
}

auto U16String::displayWidth() const noexcept -> int {
    return impl::U16StringReadTools{dataView()}.displayWidth();
}

auto U16String::indexAt(const StringSide side) const noexcept -> unit::U16DataIndex {
    return side == StringSide::Front ? unit::U16DataIndex::zero() : unit::U16DataIndex::end(length());
}

auto U16String::charAt(const StringSide side) const noexcept -> Char {
    if (isEmpty()) {
        return Char::null();
    }
    if (side == StringSide::Front) {
        return charAt(unit::U16DataIndex::zero());
    }
    auto index = indexAt(StringSide::Back);
    if (!retreat(index)) {
        return Char::null();
    }
    return charAt(index);
}

auto U16String::charAt(const unit::U16DataIndex startIndex) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.charAt(startIndex);
}

auto U16String::readCharAndAdvance(unit::U16DataIndex &index) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.read(index);
}

auto U16String::readCharAndRetreat(unit::U16DataIndex &index) const noexcept -> Char {
    return impl::U16StringReadTools{dataView()}.readAndRetreat(index);
}

auto U16String::charAt(const unit::CpIndex index) const noexcept -> Char {
    return impl::U16StringCharReadTool{dataView()}.charAt(index);
}

auto U16String::operator[](const unit::U16DataIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16String::operator[](const unit::CpIndex index) const noexcept -> Char {
    return charAt(index);
}

auto U16String::advance(unit::U16DataIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.advance(index, count);
}

auto U16String::retreat(unit::U16DataIndex &index, const unit::CpLength count) const noexcept -> bool {
    return impl::U16StringReadTools{dataView()}.retreat(index, count);
}

auto U16String::indexAt(const unit::CpIndex index) const noexcept -> unit::U16DataIndex {
    return impl::U16StringCharReadTool{dataView()}.byteIndexAt(index);
}

auto U16String::toCharIndex(const unit::U16DataIndex index) const noexcept -> unit::CpIndex {
    return impl::U16StringCharReadTool{dataView()}.charIndexAt(index);
}

auto U16String::slice(const unit::U16DataRange range) const noexcept -> U16String {
    return withRange(impl::U16StringReadTools{dataView()}.sliceRange(range));
}

auto U16String::slice(const unit::CpRange range) const noexcept -> U16String {
    return withRange(impl::U16StringCharReadTool{dataView()}.sliceRange(range));
}

auto U16String::slice(const StringSide side, const unit::U16DataLength length) const noexcept -> U16String {
    if (side == StringSide::Front) {
        return slice(unit::U16DataRange(unit::U16DataIndex::zero(), length));
    }
    const auto fullLength = this->length();
    if (length.isInfinite() || length >= fullLength) {
        return slice(unit::U16DataRange::fromLength(fullLength));
    }
    const auto start = unit::U16DataIndex::end(fullLength - length);
    return slice(unit::U16DataRange{start, length});
}

auto U16String::slice(const StringSide side, const unit::U16DataIndex index) const noexcept -> U16String {
    const auto end = indexAt(StringSide::Back);
    if (index.isNoIndex() || index >= end) {
        return side == StringSide::Front ? slice(unit::U16DataRange{unit::U16DataIndex::zero(), end}) : U16String{};
    }
    if (side == StringSide::Front) {
        return slice(unit::U16DataRange{unit::U16DataIndex::zero(), index});
    }
    return slice(unit::U16DataRange{index, end});
}

auto U16String::slice(const StringSide side, const unit::CpLength length) const noexcept -> U16String {
    if (side == StringSide::Front) {
        return slice(unit::CpRange{unit::CpIndex::zero(), length});
    }
    auto start = indexAt(StringSide::Back);
    retreat(start, length);
    return slice(unit::U16DataRange{start, indexAt(StringSide::Back)});
}

auto U16String::slice(const StringSide side, const unit::CpIndex index) const noexcept -> U16String {
    return slice(side, indexAt(index));
}

auto U16String::slice(const StringSide side) const noexcept -> std::tuple<Char, U16String> {
    if (isEmpty()) {
        return {Char::endOfData(), {}};
    }
    if (side == StringSide::Front) {
        auto end = unit::U16DataIndex::zero();
        const auto character = charAt(end);
        advance(end);
        return {character, slice(unit::U16DataRange{end, indexAt(StringSide::Back)})};
    }
    auto start = indexAt(StringSide::Back);
    retreat(start);
    return {charAt(start), slice(unit::U16DataRange{indexAt(StringSide::Front), start})};
}

auto U16String::splitAt(const unit::U16DataIndex index) const noexcept -> std::pair<U16String, U16String> {
    return {slice(StringSide::Front, index), slice(StringSide::Back, index)};
}

auto U16String::splitAt(const unit::CpIndex index) const noexcept -> std::pair<U16String, U16String> {
    return splitAt(indexAt(index));
}

}
