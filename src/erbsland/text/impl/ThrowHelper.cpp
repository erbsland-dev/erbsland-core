// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThrowHelper.hpp"

#include "../EncodingError.hpp"
#include "../FormatError.hpp"
#include "../ParseNumberError.hpp"
#include "../U16EncodingError.hpp"
#include "../U32EncodingError.hpp"
#include "../U8EncodingError.hpp"

#include "../../err/OutOfRangeFailure.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../unit/CpIndex.hpp"

namespace erbsland::text::impl {

using namespace err;

void throwEncodingError(const std::string_view reason) {
    throw EncodingError{reason};
}

void throwU8EncodingError(const std::string_view reason, const std::size_t index) {
    throw U8EncodingError{reason, unit::ByteIndex::fromSizeT(index)};
}

void throwU16EncodingError(const std::string_view reason, const std::size_t index) {
    throw U16EncodingError{reason, unit::U16Index::fromSizeT(index)};
}

void throwU32EncodingError(const std::string_view reason, const std::size_t index) {
    throw U32EncodingError{reason, unit::CpIndex::fromSizeT(index)};
}

void throwFormatError(const std::string_view reason) {
    throw FormatError{reason};
}

void throwOutOfRangeFailure(const std::string_view reason) {
    throw OutOfRangeFailure{reason};
}

void throwOverflow(const std::string_view reason) {
    throw OverflowError{reason};
}

void throwParseError(const std::string_view reason) {
    throw ParseError{reason};
}

void throwParseNumberError(const std::string_view reason, const ReadNumberStatus status) {
    throw ParseNumberError{String{reason}, status};
}

void throwParameterFailure(const std::string_view reason, const std::string_view parameterName) {
    throw ParameterFailure{reason, parameterName};
}

}
