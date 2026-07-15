// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThrowHelper.hpp"

#include "../EncodingError.hpp"
#include "../FormatError.hpp"
#include "../U16EncodingError.hpp"
#include "../U32EncodingError.hpp"
#include "../U8EncodingError.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../unit/CpIndex.hpp"

namespace erbsland::text::impl {

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

void throwFormatError(StringView reason) {
    throw FormatError{std::move(reason)};
}

void throwOutOfRange(const std::string_view reason) {
    throw err::OutOfRangeError{reason};
}

void throwOutOfRange(StringView reason) {
    throw err::OutOfRangeError{std::move(reason)};
}

void throwOverflow(const std::string_view reason) {
    throw err::OverflowError{reason};
}

void throwOverflow(StringView reason) {
    throw err::OverflowError{std::move(reason)};
}

void throwParseError(const std::string_view reason) {
    throw err::ParseError{reason};
}

void throwParseError(StringView reason) {
    throw err::ParseError{std::move(reason)};
}

}
