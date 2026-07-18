// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPattern.hpp"

#include "impl/StringPatternData.hpp"
#include "impl/StringPatternFactory.hpp"

namespace erbsland::text {

using namespace unit;

StringPattern::StringPattern(const U8String &pattern) : _data{impl::createStringPatternData(pattern)} {
}

StringPattern::StringPattern(const U16String &pattern) : _data{impl::createStringPatternData(pattern)} {
}

StringPattern::StringPattern(const U32String &pattern) : _data{impl::createStringPatternData(pattern)} {
}

auto StringPattern::matches(const U8String &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::matches(const U16String &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::matches(const U32String &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::trim(U8String &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U16String &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U32String &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U8StringEditor &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U16StringEditor &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U32StringEditor &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trimmed(const U8String &text) const noexcept -> U8String {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::trimmed(const U16String &text) const noexcept -> U16String {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::trimmed(const U32String &text) const noexcept -> U32String {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::split(const U8String &text) const noexcept -> std::pair<U8String, U8String> {
    return _data != nullptr ? _data->split(text) : std::pair<U8String, U8String>{U8String{}, text};
}

auto StringPattern::split(const U16String &text) const noexcept -> std::pair<U16String, U16String> {
    return _data != nullptr ? _data->split(text) : std::pair<U16String, U16String>{U16String{}, text};
}

auto StringPattern::split(const U32String &text) const noexcept -> std::pair<U32String, U32String> {
    return _data != nullptr ? _data->split(text) : std::pair<U32String, U32String>{U32String{}, text};
}

auto StringPattern::length(const U8String &text) const noexcept -> ByteLength {
    return _data != nullptr ? _data->length(text) : ByteLength::zero();
}

auto StringPattern::length(const U16String &text) const noexcept -> U16DataLength {
    return _data != nullptr ? _data->length(text) : U16DataLength::zero();
}

auto StringPattern::length(const U32String &text) const noexcept -> CpLength {
    return _data != nullptr ? _data->length(text) : CpLength::zero();
}

auto StringPattern::index(const U8String &text) const noexcept -> ByteIndex {
    return _data != nullptr ? _data->index(text) : ByteIndex::noIndex();
}

auto StringPattern::index(const U16String &text) const noexcept -> U16DataIndex {
    return _data != nullptr ? _data->index(text) : U16DataIndex::noIndex();
}

auto StringPattern::index(const U32String &text) const noexcept -> CpIndex {
    return _data != nullptr ? _data->index(text) : CpIndex::noIndex();
}

}
