// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThrowHelper.hpp"

#include "FormatError.hpp"
#include "OutOfRangeError.hpp"
#include "OverflowError.hpp"
#include "ParseError.hpp"
#include "U16EncodingError.hpp"
#include "U32EncodingError.hpp"
#include "U8EncodingError.hpp"

#include "../unit/CpIndex.hpp"

namespace erbsland::err {

void throwEncodingError(const std::string_view reason) {
    throw EncodingError{reason};
}

void throwU8EncodingError(const std::string_view reason, const std::size_t index) {
    throw U8EncodingError{reason, unit::ByteIndex::fromSizeT(index)};
}

void throwU16EncodingError(const std::string_view reason, const std::size_t index) {
    throw U16EncodingError{reason, unit::U16DataIndex::fromSizeT(index)};
}

void throwU32EncodingError(const std::string_view reason, const std::size_t index) {
    throw U32EncodingError{reason, unit::CpIndex::fromSizeT(index)};
}

void throwFormatError(const std::string_view reason) {
    throw FormatError{reason};
}

void throwFormatError(text::StringView reason) {
    throw FormatError{std::move(reason)};
}

void throwOutOfRange(const std::string_view reason) {
    throw OutOfRangeError{reason};
}

void throwOutOfRange(text::StringView reason) {
    throw OutOfRangeError{std::move(reason)};
}

void throwOverflow(const std::string_view reason) {
    throw OverflowError{reason};
}

void throwOverflow(text::StringView reason) {
    throw OverflowError{std::move(reason)};
}

void throwParseError(const std::string_view reason) {
    throw ParseError{reason};
}

void throwParseError(text::StringView reason) {
    throw ParseError{std::move(reason)};
}

}
