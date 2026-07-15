// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPattern.hpp"

#include "impl/StringPatternData.hpp"
#include "impl/StringPatternFactory.hpp"

namespace erbsland::text {

StringPattern::StringPattern(const U8StringView &pattern) : _data{impl::createStringPatternData(pattern)} {
}

StringPattern::StringPattern(const U16StringView &pattern) : _data{impl::createStringPatternData(pattern)} {
}

StringPattern::StringPattern(const U32StringView &pattern) : _data{impl::createStringPatternData(pattern)} {
}

auto StringPattern::matches(const U8StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::matches(const U16StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::matches(const U32StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->matches(text);
}

auto StringPattern::trim(U8StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U16StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U32StringView &text) const noexcept -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U8String &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U16String &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trim(U32String &text) const -> bool {
    return _data != nullptr && _data->trim(text);
}

auto StringPattern::trimmed(const U8StringView &text) const noexcept -> U8StringView {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::trimmed(const U16StringView &text) const noexcept -> U16StringView {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::trimmed(const U32StringView &text) const noexcept -> U32StringView {
    return _data != nullptr ? _data->trimmed(text) : text;
}

auto StringPattern::split(const U8StringView &text) const noexcept -> std::pair<U8StringView, U8StringView> {
    return _data != nullptr ? _data->split(text) : std::pair<U8StringView, U8StringView>{U8StringView{}, text};
}

auto StringPattern::split(const U16StringView &text) const noexcept -> std::pair<U16StringView, U16StringView> {
    return _data != nullptr ? _data->split(text) : std::pair<U16StringView, U16StringView>{U16StringView{}, text};
}

auto StringPattern::split(const U32StringView &text) const noexcept -> std::pair<U32StringView, U32StringView> {
    return _data != nullptr ? _data->split(text) : std::pair<U32StringView, U32StringView>{U32StringView{}, text};
}

auto StringPattern::length(const U8StringView &text) const noexcept -> unit::ByteLength {
    return _data != nullptr ? _data->length(text) : unit::ByteLength::zero();
}

auto StringPattern::length(const U16StringView &text) const noexcept -> unit::U16DataLength {
    return _data != nullptr ? _data->length(text) : unit::U16DataLength::zero();
}

auto StringPattern::length(const U32StringView &text) const noexcept -> unit::CpLength {
    return _data != nullptr ? _data->length(text) : unit::CpLength::zero();
}

auto StringPattern::index(const U8StringView &text) const noexcept -> unit::ByteIndex {
    return _data != nullptr ? _data->index(text) : unit::ByteIndex::noIndex();
}

auto StringPattern::index(const U16StringView &text) const noexcept -> unit::U16DataIndex {
    return _data != nullptr ? _data->index(text) : unit::U16DataIndex::noIndex();
}

auto StringPattern::index(const U32StringView &text) const noexcept -> unit::CpIndex {
    return _data != nullptr ? _data->index(text) : unit::CpIndex::noIndex();
}

}
